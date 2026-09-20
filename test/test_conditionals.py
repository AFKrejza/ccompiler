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
	])
	def test_ifs(self, compile_and_run, input, expected):
		assert compile_and_run(input) == expected

	# @pytest.mark.parametrize("input", [
	# 	(textwrap.dedent("""
	# 	int main()
	# 	{
	# 		return a;
	# 	}
	# 	""")),
	# 	(textwrap.dedent("""
	# 	int main()
	# 	{
	# 		b = a;
	# 		int a = 5;
	# 		int b = 2;
	# 		return a;
	# 	}
	# 	""")),
	# 	(textwrap.dedent("""
	# 	int main()
	# 	{
	# 		int int;
	# 	}
	# 	""")),
	# 	(textwrap.dedent("""
	# 	int main()
	# 	{
	# 		return a;
	# 	}
	# 	""")),
	# ])
	# def test_vars_compiler_errors(self, compile_fail, input):
	# 	assert compile_fail(input) != 0

	# @pytest.mark.parametrize("input, expected", [
	# 	(textwrap.dedent("""
	# 	int main()
	# 	{
	# 		int a = -2;
	# 		return a;
	# 	}
	# 	"""), 254),
	# 	(textwrap.dedent("""
	# 	int main()
	# 	{
	# 		int a = -2;
	# 		return a + -a;
	# 	}
	# 	"""), 0),
	# 	(textwrap.dedent("""
	# 	int main()
	# 	{
	# 		return -1;
	# 	}
	# 	"""), 255),
	# ])
	# def test_negatives(self, compile_and_run, input, expected):
	# 	assert compile_and_run(input) == expected
		