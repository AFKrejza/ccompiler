"""
(textwrap.dedent("""

"""), 4),

(textwrap.dedent("""

""")),
"""

# pytest test/test_functions.py::TestFunctions::test_funcs

import pytest
import textwrap

class TestFunctions:

	@pytest.mark.parametrize("input, expected", [
	(textwrap.dedent("""
	int one()
	{
		return 1;
	}

	int main()
	{
		return one();
	}
	"""), 1),
	(textwrap.dedent("""
	int smirk(int a)
	{
		return a + 1;
	}
	int main()
	{
		return smirk(1);
	}
	"""), 2),
	(textwrap.dedent("""
int factorial(int n)
{
	int num = n;
	while (n > 1)
	{
		n = n - 1;
		num = num * n;
	}
	return num;
}

int main()
{
	return factorial(5);
}
	"""), 120),
	(textwrap.dedent("""
int func(int a, int b, int c, int d, int e, int f)
{
	int g;
	g = a + b + c + d + e + f;
	return g;
}

int main()
{
	int a = func(1,2,3,4,5,6);

	return a;
}

	"""), 21),
	(textwrap.dedent("""
int nested()
{
	return 1;
}
int another()
{
	return nested();
}
int func()
{
	return another();
}
int main()
{
	return func();
}

	"""), 1),
	(textwrap.dedent("""
int square(int x) {
	return x * x;
}
int main() {
	int a = square(5);
	int b = square(3);
	return a + b;
}
	"""), 34),
	(textwrap.dedent("""
int add(int a, int b) {
	return a + b;
}
int triple(int x) {
	return x + x + x;
}
int main() {
	return add(triple(4), triple(6));
}
	"""), 30),
	(textwrap.dedent("""
int sq(int x) {
	return x * x;
}
int main() {
	int total = 0;
	for (int i = 1; i <= 4; i = i + 1) {
		total = total + sq(i);
	}
	return total;
}
	"""), 30),
	(textwrap.dedent("""
int max(int a, int b)
{
	if (a > b)
		return a;
	else
		return b;
}

int min(int a, int b)
{
	if (a < b)
		return a;
	else
		return b;
}
int clamp(int x, int lo, int hi) {
	return min(max(x, lo), hi);
}
int main() {
	int a = clamp(50, 0, 10);
	int b = clamp(-5, 0, 10);
	int c = clamp(7, 0, 10);
	return a + b + c;
}
	"""), 17),
	(textwrap.dedent("""
int is_big(int x)
{
	if (x > 100)
		return 1;
	else
		return 0;
}
int main()
{
	if (is_big(200))
		return 7;
	else
		return 9;
}
	"""), 7),
	(textwrap.dedent("""
int add(int a, int b)
{
	return a + b;
}
int main()
{
	return add(add(1, 2), add(3, 4));
}
	"""), 10),
	(textwrap.dedent("""
int weighted(int a, int b, int c)
{
	int r = a * 100 + b * 10 + c;
	return r;
}
int main()
{
	return weighted(1, 2, 3);
}
	"""), 123),
	(textwrap.dedent("""
int inc(int x)
{
	return x + 1;
}
int twice(int x)
{
	return x + x;
}
int main()
{
	int a = inc(4);
	int b = twice(a);
	return twice(b) + inc(a);
}
	"""), 26),
	(textwrap.dedent("""
int limit(int x)
{
	return x * 3;
}
int main()
{
	int sum = 0;
	for (int i = 0; i < 10; i = i + 1)
	{
		if (sum > limit(10))
			break;
		sum = sum + i;
	}
	return sum;
}
	"""), 36),
	(textwrap.dedent("""
int combine(int a, int b, int c, int d, int e, int f)
{
	return a * 1 + b * 2 + c * 3 + d * 4 + e * 5 + f * 6;
}
int main()
{
	return combine(6, 5, 4, 3, 2, 1);
}
	"""), 56),
	])
	def test_funcs(self, compile_and_run, input, expected):
		assert compile_and_run(input) == expected


	@pytest.mark.parametrize("input", [
	(textwrap.dedent("""
int main()
{
	return smirk();
}

int smirk()
{
	return 1;
}
	""")),
	(textwrap.dedent("""
int func()
{
	return 1;
}
	""")),
	(textwrap.dedent("""
int main()
{
	return ghost(3);
}
	""")),
	(textwrap.dedent("""
int f(int a)
{
	return a;
}
int main()
{
	return f(1, 2);
}
	""")),
	(textwrap.dedent("""
int add(int a, int b)
{
	return a + b;
}
int main()
{
	return add(1);
}
	""")),
	(textwrap.dedent("""
int main()
{
	return helper(2);
}
int helper(int x)
{
	return x + x;
}
	""")),
	(textwrap.dedent("""
int f()
{
	return 1;
}
int f()
{
	return 2;
}
int main()
{
	return f();
}
	""")),
	(textwrap.dedent("""
int f()
{
	return x;
}
int main()
{
	return f();
}
	""")),
(textwrap.dedent("""
int f(int a, int b, int c, int d, int e, int g, int h)
{
	return a;
}
int main()
{
	return f(1, 2, 3, 4, 5, 6, 7);
}
	""")),
	])
	def test_functions_compiler_errors(self, compile_fail, input):
		assert compile_fail(input) != 0

"""
later this should fail:
int f(int x)
{
	return 1;
}
int main()
{
	return f(2);
}
"""