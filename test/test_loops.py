import pytest
import textwrap

class TestConditionals:

	@pytest.mark.parametrize("input, expected", [
	(textwrap.dedent("""
	int main()
	{
		int a = 2;
		int i = 2;
		while (i)
		{
			a = a + 1;
			i = i - 1;
		}
		return a;
	}
	"""), 4),
	(textwrap.dedent("""
	int main()
	{
		while (0)
		{

		}
		return 1;
	}
	"""), 1),
	(textwrap.dedent("""
	int main()
	{
		int i = 5;
		while (i)
		{
			if (i == 1)
				break;

			i = i - 1;
		}
		return i;
	}
	"""), 1),
	(textwrap.dedent("""
	int main()
	{
		int i = -5;
		while (i < 10)
		{
			i = i + 1;
			if (i < 0)
				continue;
			return i;
		}
		return i;
	}
	"""), 0),
	(textwrap.dedent("""
	int main() {
		int i = 0;
		while (i < 100) {
			if (i == 7)
				break;
			i = i + 1;
		}
		return i;
	}
	"""), 7),
	(textwrap.dedent("""
	int main() {
		int i = 1;
		int sum = 0;
		for (int i = 1; i <= 3; i = i + 1) {
			for (int j = 1; j <= 3; j = j + 1) {
				sum = sum + i * j;
			}
		}
		return sum;
	}
	"""), 36),
	])
	def test_while(self, compile_and_run, input, expected):
		assert compile_and_run(input) == expected

	@pytest.mark.parametrize("input, expected", [
	(textwrap.dedent("""
	int main()
	{
		int a = 0;
		do {
			a = a + 1;
		} while (a < 10);

		return a;
	}
	"""), 10),
	(textwrap.dedent("""
	int main()
	{
		int a = 0;
		do {
			while (a < 10) {
				a = a + 1;
			}
			a = a + 1;
		} while (a < 10);

		return a;
	}
	"""), 11),
	(textwrap.dedent("""
	int main()
	{
		int found = 0;
		int result = 0;
		int i = 1;
		do {
			int j = 1;
			while (j <= 5)
			{
				if (i * j >= 12)
				{
					result = i * 10 + j;
					found = 1;
					break;
				}
				j = j + 1;
			}
			if (found == 1)
				break;
			i = i + 1;
		} while (i <= 5);
		return result;
	}
	"""), 34),
	(textwrap.dedent("""
	int main()
	{
		int total = 0;
		int x = 2;
		for (int i = 1; i <= 10; i = i + 1)
		{
			int x = i * i;
			if (x > 50)
				break;
			if (x < 10)
				continue;
			total = total + x;
		}
		return total + x;
	}
	"""), 128),
	])
	def test_dowhile(self, compile_and_run, input, expected):
			assert compile_and_run(input) == expected


	@pytest.mark.parametrize("input, expected", [
	(textwrap.dedent("""
	int main()
	{
		int a = 0;
		for (int i = 0; i < 5; i = i + 1)
		{
			a = a + 1;
		}
		return a;
	}
	"""), 5),
	(textwrap.dedent("""
	int main()
	{
		for (;;)
		{
			return 1;
		}
	}
	"""), 1),
	(textwrap.dedent("""
	int main()
	{
		for (int i;;)
		{
			return 1;
		}
	}
	"""), 1),
	(textwrap.dedent("""
	int main()
	{
		for (;1;)
		{
			return 1;
		}
	}
	"""), 1),
	(textwrap.dedent("""
	int main()
	{
		int a = 0;
		for (;;a = a + 1)
		{
			if (a == 3)
				break;
		}
		return a;
	}
	"""), 3),
	(textwrap.dedent("""
	int main()
	{
		int i = 0;
		for (;i < 2; i = i + 1)
		{
			return 1;
		}
	}
	"""), 1)
	])
	def test_for(self, compile_and_run, input, expected):
			assert compile_and_run(input) == expected

	@pytest.mark.parametrize("input", [
	(textwrap.dedent("""
	int main()
	{
		while (0) {
			return a;
		}
		return 1;
	}
	""")),
	(textwrap.dedent("""
	int main()
	{
		for ()
	}
	""")),
	(textwrap.dedent("""
	int main()
	{
		int a = 0;
		do
		{
			a = a + 1;
			if (a == 2) break;
		}
		while ();
		return 1;
	}
	""")),
	(textwrap.dedent("""
	int main()
	{
		while ()
		{
			break;
		}
		return 1;
	}
	"""))
	])
	def test_vars_compiler_errors(self, compile_fail, input):
		assert compile_fail(input) != 0




