import pytest
import textwrap

from constants import u_max, i_max

# Various variable test. Do LOTS of failures as well. Sandler's book has plenty of ideas.
# Perhaps check her repo for more tests?
class TestVariables:

	# def test_empty_return(self, compile_fail):
	# 	source = "int main () { return ; }"
	# 	assert compile_fail(source) != 0

	@pytest.mark.parametrize("input, expected", [
		(textwrap.dedent("""
			int main()
			{
				int a = 5;
				return a;
			}
		"""), 5),
		(textwrap.dedent("""
			int main()
			{
				int a = 1;
				int b = 2;
				int c = a + b;
				return c;
			}
				"""), 3),
		(textwrap.dedent("""
			int main()
			{
				int a = 1;
				int b = a;
				return b;
			}
				"""), 1),
	])
	def test_vars(self, compile_and_run, input, expected):
		assert compile_and_run(input) == expected

# test failures