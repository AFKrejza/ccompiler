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

	# TODO: This should be parametrized (ironically), separate for returning 1 or 0
	@pytest.mark.parametrize("input, expected", [
		(textwrap.dedent("""
		int main()
		{
			int a = 1;
			a = a == a;
			return a;
		}
		"""), 1),
		(textwrap.dedent("""
		int main()
		{
			int a = 1;
			return a == 0;
		}
		"""), 0),
		(textwrap.dedent("""
		int main()
		{
			int a = 1;
			int b = 0;
			return a || b;
		}
		"""), 1),
		(textwrap.dedent("""
		int main()
		{
			int a = 1;
			int b = 0;
			return b || a;
		}
		"""), 1),
		(textwrap.dedent("""
		int main()
		{
			int a = 1;
			int b = 1;
			return a || b;
		}
		"""), 1),
		(textwrap.dedent("""
		int main()
		{
			int a = 0;
			int b = 0;
			return a || b;
		}
		"""), 0),
		(textwrap.dedent("""
		int main()
		{
			int a = 0;
			int b = 0;
			return a && b;
		}
		"""), 0),
		(textwrap.dedent("""
		int main()
		{
			int a = 1;
			int b = 1;
			return a && b;
		}
		"""), 1),
		(textwrap.dedent("""
		int main()
		{
			int a = 1;
			int b = 0;
			return a && b;
		}
		"""), 0),
		(textwrap.dedent("""
		int main()
		{
			int a = 0;
			int b = 1;
			return a && b;
		}
		"""), 0),
		(textwrap.dedent("""
		int main()
		{
			int a = 0;
			int b = 0;
			return a != b;
		}
		"""), 0),
		(textwrap.dedent("""
		int main()
		{
			int a = 1;
			int b = 0;
			return a != b;
		}
		"""), 1),
		(textwrap.dedent("""
		int main()
		{
			int a = 1;
			int b = 0;
			return a < b;
		}
		"""), 0),
		(textwrap.dedent("""
		int main()
		{
			int a = 0;
			int b = 1;
			return a < b;
		}
		"""), 1),
		(textwrap.dedent("""
		int main()
		{
			int a = 1;
			int b = 0;
			return a <= b;
		}
		"""), 0),
		(textwrap.dedent("""
		int main()
		{
			int a = 1;
			int b = 1;
			return a <= b;
		}
		"""), 1),
		(textwrap.dedent("""
		int main()
		{
			int a = 1;
			int b = 2;
			return a <= b;
		}
		"""), 1),


		(textwrap.dedent("""
		int main()
		{
			int a = 0;
			int b = 1;
			return a > b;
		}
		"""), 0),
		(textwrap.dedent("""
		int main()
		{
			int a = 1;
			int b = 0;
			return a > b;
		}
		"""), 1),
		(textwrap.dedent("""
		int main()
		{
			int a = 0;
			int b = 1;
			return a >= b;
		}
		"""), 0),
		(textwrap.dedent("""
		int main()
		{
			int a = 1;
			int b = 1;
			return a >= b;
		}
		"""), 1),
		(textwrap.dedent("""
		int main()
		{
			int a = 2;
			int b = 1;
			return a >= b;
		}
		"""), 1),
	])
	def test_binary_operators(self, compile_and_run, input, expected):
		#	==	||	&&	 !=	 <	 <=	 >	 >=
		assert compile_and_run(input) == expected


	# extra test for logical OR
	def test_or(self, compile_and_run):
		code = textwrap.dedent("""
		int main()
		{
			int a = 2;
			int b = a * 5;
			int c = a || b;
			a = 0;
			b = a && c;

			return b < c; 
		}
		""")
		assert compile_and_run(code) == 1

		code = textwrap.dedent("""
		int main()
		{
			int a = 2;
			int b = 5;
			int c = a || b;
			return c;
		}
		""")
		assert compile_and_run(code) == 1