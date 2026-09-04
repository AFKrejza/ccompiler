import pytest
import textwrap

from constants import u_max, i_max

class TestExpressions:

	@pytest.mark.parametrize("input, expected", [
		(textwrap.dedent("""
		int main()
		{
			return (1);
		}
		"""), 1),
		(textwrap.dedent("""
		int main()
		{
			return (1 + 2);
		}
		"""), 3),
		(textwrap.dedent("""
		int main()
		{
			return (((((2 + 3)))));
		}
		"""), 5),
		(textwrap.dedent("""
		int main()
		{
			return 1 + (9);
		}
		"""), 10),
		(textwrap.dedent("""
		int main()
		{
			return (2) + (2);
		}
		"""), 4),
		(textwrap.dedent("""
		int main()
		{
			return ((2) + (2));
		}
		"""), 4),
		(textwrap.dedent("""
		int main()
		{
			return (1 + 1) + 2;
		}
		"""), 4)
	])
	def test_vars(self, compile_and_run, input, expected):
		assert compile_and_run(input) == expected

	@pytest.mark.parametrize("input", [
		(textwrap.dedent("""
		int main()
		{
			return ();
		}
		""")),
		(textwrap.dedent("""
		int main()
		{
			return (;
		}
		""")),
		(textwrap.dedent("""
		int main()
		{
			return );
		}
		""")),
		(textwrap.dedent("""
		int main()
		{
			return (1 + (2);
		}
		""")),
		(textwrap.dedent("""
		int main()
		{
			return (( 1 + 1);
		}
		""")),
		(textwrap.dedent("""
		int main()
		{
			return 1 + 2)));
		}
		""")),
	])
	def test_vars_compiler_errors(self, compile_fail, input):
		assert compile_fail(input) != 0

	@pytest.mark.parametrize("input, expected", [
		(textwrap.dedent("""
		int main()
		{
			return 2 * 2;
		}
		"""), 4),
		(textwrap.dedent("""
		int main()
		{
			int a = 1;
			int b = 2;
			return a * (b * b);
		}
		"""), 4),
		(textwrap.dedent("""
		int main()
		{
			return 2 * (5 * 5);
		}
		"""), 50),
	])
	def test_mul(self, compile_and_run, input, expected):
		assert compile_and_run(input) == expected