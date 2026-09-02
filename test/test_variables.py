import pytest
import textwrap

from constants import u_max, i_max

# Various variable tests. Do LOTS of failures as well. Sandler's book has plenty of ideas.
class TestVariables:

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
		(textwrap.dedent("""
		int main()
		{
			int a;
			a = 5;
			return a;
		}
		"""), 5	)
	])
	def test_vars(self, compile_and_run, input, expected):
		assert compile_and_run(input) == expected

	@pytest.mark.parametrize("input", [
		(textwrap.dedent("""
		int main()
		{
			return a;
		}
		""")),
		(textwrap.dedent("""
		int main()
		{
			b = a;
			int a = 5;
			int b = 2;
			return a;
		}
		""")),
		(textwrap.dedent("""
		int main()
		{
			int int;
		}
		""")),
		(textwrap.dedent("""
		int main()
		{
			return a;
		}
		""")),
	])
	def test_vars_compiler_errors(self, compile_fail, input):
		assert compile_fail(input) != 0
