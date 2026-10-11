"""
Tests the extern keyword on functions.
"""


import pytest
import textwrap

class TestExtern:
	@pytest.mark.parametrize("input, expected, stdout", [
		(("""
		extern int putchar (int __c);
		int main()
		{
			putchar(65);
			return 0;
		}
		"""), 0, "A"),
		(("""
		extern int main();
		extern int main() {
			return 1;
		}
		""", 1, "")),
		(("""
		extern int ext (int n);
		int ext(int n) {
			return n;
		}
		int main() {
			ext(1);
			return 0;
		}
		"""), 0, ""),
	])
	def test_recursion(self, capsys, compile_and_run, input, expected, stdout):
		assert compile_and_run(textwrap.dedent(input)) == expected
		captured = capsys.readouterr()
		assert captured.out == stdout
