import unittest
from scripts.evidence import check_suite


class TestEvidence(unittest.TestCase):
    def test_pass(self):
        self.assertEqual(len(check_suite("PASS T1 clean\nSUMMARY pass=1 fail=0 inconclusive=0\n")), 1)

    def test_reject_false_success(self):
        for output in ("PASS T1 clean\n", "SUMMARY pass=0 fail=0 inconclusive=0\n",
                       "FAIL T1 bad\nSUMMARY pass=1 fail=0 inconclusive=0\n",
                       "INCONCLUSIVE T13 reuse\nSUMMARY pass=0 fail=0 inconclusive=1\n",
                       "PASS T1 clean\nSUMMARY pass=1 fail=0 inconclusive=0\nSUMMARY pass=1 fail=0 inconclusive=0\n"):
            with self.subTest(output=output), self.assertRaises(ValueError):
                check_suite(output)
