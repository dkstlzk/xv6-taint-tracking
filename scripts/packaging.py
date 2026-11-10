"""Member A: preliminary A-21 packages from immutable Git blobs."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import zipfile

from scripts.evidence import ROOT

BASELINE = "f780d7850765d74cbdcd1fff4b822d8829f7c9bd"


def git(*args):
    return subprocess.check_output(["git", *args], cwd=ROOT)


def revision(ref):
    if ref.startswith("-"):
        raise ValueError("invalid revision")
    return git("rev-parse", "--verify", ref + "^{commit}").decode().strip()


def changed(commit):
    return set(git("diff", "--name-only", "-z", BASELINE, commit, "--").decode().split("\0")) - {""}


_ATTR_CACHE = {}


def attribution(commit):
    if commit in _ATTR_CACHE:
        return _ATTR_CACHE[commit]
    owners = {member: {} for member in "ABCD"}
    raw = git("log", "--format=COMMIT:%H%x09%s", "--name-only", "-z", commit, "--").decode()
    current_member = None
    current_sha = None
    current_subject = None
    for token in raw.split("\0"):
        token = token.strip("\n")
        if not token:
            continue
        if token.startswith("COMMIT:"):
            line = token[len("COMMIT:"):]
            parts = line.split("\t", 1)
            current_sha = parts[0]
            current_subject = parts[1] if len(parts) > 1 else ""
            match = re.search(r"\[Member ([ABCD])\]", current_subject)
            current_member = match.group(1) if match else None
        elif current_member:
            owners[current_member].setdefault(token, []).append({"commit": current_sha, "subject": current_subject})
    _ATTR_CACHE[commit] = owners
    return owners


def pack(member, ref, output, team=False):
    commit = revision(ref)
    owners = attribution(commit)
    paths = changed(commit)
    if not team:
        paths &= owners[member].keys()
    current = set(git("ls-tree", "-r", "--name-only", "-z", commit).decode().split("\0"))
    manifest = {"commit": commit, "baseline": BASELINE, "member": "team" if team else member,
                "attribution": "assigned member from commit subject; not proof of personal authorship",
                "files": {}, "deleted": sorted(paths - current)}
    output = Path(output)
    output.parent.mkdir(parents=True, exist_ok=True)
    notes = [f"# Changes at {commit}", "", manifest["attribution"], ""]
    with zipfile.ZipFile(output, "w", zipfile.ZIP_DEFLATED) as archive:
        for path in sorted(paths & current):
            data = git("show", f"{commit}:{path}")
            manifest["files"][path] = hashlib.sha256(data).hexdigest()
            entry = zipfile.ZipInfo("files/" + path)
            entry.date_time = (1980, 1, 1, 0, 0, 0)
            mode = int(git("ls-tree", commit, "--", path).split()[0], 8)
            entry.external_attr = mode << 16
            entry.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(entry, data)
            notes.append(f"- `{path}`")
            for owner in ("ABCD" if team else member):
                for change in owners[owner].get(path, []):
                    notes.append(f"  - {owner}: {change['commit'][:12]} {change['subject']}")
        archive.writestr("manifest.json", json.dumps(manifest, indent=2, sort_keys=True) + "\n")
        archive.writestr("changes.md", "\n".join(notes) + "\n")
    verify(output, ref)
    return output


def verify(path, ref=None):
    with zipfile.ZipFile(path) as archive:
        assert archive.testzip() is None, "corrupt ZIP"
        manifest = json.loads(archive.read("manifest.json"))
        commit = revision(ref) if ref else manifest["commit"]
        assert manifest["commit"] == commit and manifest["baseline"] == BASELINE
        expected = changed(commit)
        if manifest["member"] != "team":
            expected &= attribution(commit)[manifest["member"]].keys()
        current = set(git("ls-tree", "-r", "--name-only", "-z", commit).decode().split("\0"))
        assert set(manifest["files"]) == expected & current, "incomplete package"
        assert set(manifest["deleted"]) == expected - current
        names = {"files/" + path for path in manifest["files"]} | {"manifest.json", "changes.md"}
        assert set(archive.namelist()) == names and len(archive.namelist()) == len(names), "unexpected ZIP entries"
        for filename, digest in manifest["files"].items():
            data = archive.read("files/" + filename)
            assert hashlib.sha256(data).hexdigest() == digest
            assert data == git("show", f"{commit}:{filename}"), filename
    print(f"PASS canonical byte verification: {path} ({commit})")


def main():
    parser = argparse.ArgumentParser()
    sub = parser.add_subparsers(dest="command", required=True)
    pack_parser = sub.add_parser("pack")
    pack_parser.add_argument("member", choices=list("ABCD"))
    pack_parser.add_argument("ref")
    pack_parser.add_argument("--output-dir", default="artifacts/packages")
    verify_parser = sub.add_parser("verify")
    verify_parser.add_argument("path")
    verify_parser.add_argument("ref", nargs="?", default=None)
    report_parser = sub.add_parser("attribution")
    report_parser.add_argument("ref")
    args = parser.parse_args()
    if args.command == "verify":
        verify(args.path, args.ref)
    elif args.command == "attribution":
        print(json.dumps(attribution(revision(args.ref)), indent=2))
    else:
        layout = os.environ.get("LAYOUT", "individual")
        if layout not in ("individual", "team", "both", "nested"):
            raise ValueError("LAYOUT must be individual, team, both, or nested")
        output = Path(args.output_dir)
        archives = []
        if layout != "team":
            archives.append(pack(args.member, args.ref, output / f"member_{args.member}.zip"))
        if layout != "individual":
            archives.append(pack(args.member, args.ref, output / "team.zip", team=True))
        if layout == "nested":
            with zipfile.ZipFile(output / f"bundle_{args.member}.zip", "w", zipfile.ZIP_DEFLATED) as bundle:
                for archive in archives:
                    bundle.write(archive, archive.name)


if __name__ == "__main__":
    main()
