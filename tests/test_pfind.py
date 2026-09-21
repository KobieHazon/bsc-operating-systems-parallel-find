"""Run the filename-search program against temporary directory trees."""

import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

PROGRAM = Path(sys.argv.pop(1)).resolve()


class FindTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="pfind-")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        (self.root / "nested folder").mkdir()
        (self.root / "report-one.txt").write_text("one")
        (self.root / "nested folder/report two.txt").write_text("two")
        (self.root / "ignore.txt").write_text("report in content only")

    def run_find(self, *arguments):
        return subprocess.run(
            [str(PROGRAM), *map(str, arguments)],
            text=True,
            capture_output=True,
            timeout=10,
        )

    def test_nested_search_one_and_many_workers(self):
        expected = {
            str(self.root / "report-one.txt"),
            str(self.root / "nested folder/report two.txt"),
        }
        for workers in (1, 2, 4, 16):
            with self.subTest(workers=workers):
                result = self.run_find(self.root, "report", workers)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(result.stderr, "")
                lines = result.stdout.splitlines()
                self.assertEqual(set(lines[:-1]), expected)
                self.assertEqual(lines[-1], "Done searching, found 2 files")

    def test_no_match(self):
        result = self.run_find(self.root, "NO-MATCH", 4)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout, "Done searching, found 0 files\n")

    def test_empty_directory(self):
        empty = self.root / "empty"
        empty.mkdir()
        result = self.run_find(empty, "anything", 4)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout, "Done searching, found 0 files\n")

    def test_symlink_cycle_is_not_followed(self):
        (self.root / "nested folder/loop").symlink_to(
            self.root, target_is_directory=True
        )
        result = self.run_find(self.root, "report", 4)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertTrue(result.stdout.endswith("Done searching, found 2 files\n"))

    def test_invalid_thread_counts(self):
        for workers in ("0", "-1", "two", "2oops", "", "999999999999999999999"):
            with self.subTest(workers=workers):
                result = self.run_find(self.root, "report", workers)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("THREAD_COUNT", result.stderr)

    def test_missing_directory(self):
        result = self.run_find(self.root / "missing", "report", 1)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("directory", result.stderr)

    def test_usage(self):
        result = self.run_find()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Usage:", result.stderr)


if __name__ == "__main__":
    unittest.main()
