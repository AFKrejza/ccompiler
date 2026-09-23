import pytest
import textwrap

from constants import u_max, i_max

# Various variable tests. Do LOTS of failures as well. Sandler's book has plenty of ideas.
class TestConditionals:

	@pytest.mark.parametrize("input, expected", [
		(textwrap.dedent("""
		int main()
		{
			if (1)
				return 1;
			
			return 0;
		}
		"""), 1),
		(textwrap.dedent("""
		int main()
		{
			if (0)
				return 1;

			return 0;
		}
		"""), 0),
		(textwrap.dedent("""
		int main()
		{
			if (1 || 0)
				return 1;
		
			return 0;
		}
		"""), 1),
		(textwrap.dedent("""
		int main()
		{
			int a = 1;
			if (1 || a)
				return 1;
			return 0;
		}
		"""), 1),
		(textwrap.dedent("""
		int main()
		{
			if (1) {
				int a = 10;
				return a;
			}
			return 0;
		}
		"""), 10),
		(textwrap.dedent("""
		int main()
		{
			int a = 1;
			if (a) 
			{
				a = a + 1;
				if (a) 
				{
					if (a)
						if (a)
							if (a)
								a = 7;
				}
			}
			return a;
		}
		"""), 7),
		(textwrap.dedent("""
		int main()
		{
			if (0)
				return 2;
			else return 1;
				
			return 0;
		}
		"""), 1),
		(textwrap.dedent("""
		int main()
		{
			int a = 0;
			if (a)
			{
				return 2;
			}
			else
			{
				return 1;
			}
				
			return 0;
		}
		"""), 1),
		(textwrap.dedent("""
		int main()
		{
			int a = 0;
			if (a)
			{
				return 2;
			}
			else if (!a)
			{
				return 1;
			}
				
			return 0;
		}
		"""), 1),
		(textwrap.dedent("""
		int main()
		{
			int a = 0;
			if (a)
			{
				return 2;
			}
			else if (a)
			{
				return 3;
			}
			else
			{
				return 1;
			}
				
			return 0;
		}
		"""), 1),
		(textwrap.dedent("""
		int main()
		{
			int a = 0;
			if (0)
				a = 1;
			else if (1) {
				a = 2;
			}
			else {
				a = 3;
				return a;
			}
			return 1;
		}
		"""), 3),
		(textwrap.dedent("""
		int main()
		{
			int a = 0;
			if (1)
				a = 4;
			else {
				if (1) {
					a = 5;
				}
				else {
					a = 6;
				}
				return a;
			}

			return a;
		}
		"""), 6),
	])
	def test_all(self, compile_and_run, input, expected):
		assert compile_and_run(input) == expected

	@pytest.mark.parametrize("input", [
		(textwrap.dedent("""
		int main()
		{
			else return 1;
		}
		""")),
		(textwrap.dedent("""
		int main()
		{
			if (1)
				if (2)
					else return 1;
			else return 2;
			else return 3;
		}
		""")),
		(textwrap.dedent("""
		int main()
		{
			if {
				return 5;
			}
		}
		""")),
		(textwrap.dedent("""
		int main()
		{
			if (else if (1) return 3)
		}
		""")),
	])
	def test_vars_compiler_errors(self, compile_fail, input):
		assert compile_fail(input) != 0
