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


	@pytest.mark.parametrize("input, expected", [
		(("""
		int f(int x) {
			return 1;
		}
		int main() {
			return f(2);
		}
		"""), 1),
		(("""
		int f(int n);int f(int n);int f(int n);int f(int n);int f(int n);
		int f(int n) {
			return n * 2;
		}
		int main() {
			int a = f(2);
			return a;
		}
		"""), 4),
		(("""
		int ghost(int a);
		int main() {
			return 7;
		}
		"""), 7),
		(("""
		int sq(int x) {
			return x * x;
		}
		int sq(int x);
		int main() {
			return sq(6);
		}
		"""), 36),
		(("""
		int later(int x);
		int main() {
			return later(5) + later(3);
		}
		int later(int x) {
			return x + x;
		}
		"""), 16),
		(("""
		int f(int x);
		int f(int y) {
			return y + 1;
		}
		int main() {
			return f(10);
		}
		"""), 11),
		(("""
		int main();
		int main() {
			return 1;
		}
		"""), 1),
		])
	def test_forward_decl(self, compile_and_run, input, expected):
		assert compile_and_run(textwrap.dedent(input)) == expected

	# these should pass but it's not a big deal. Might add it later.
	# forward declarations don't need parameter names and the names don't have to match the definition!
		(("""
		int s6(int, int, int, int, int, int);
		int main() {
			return s6(1, 2, 3, 4, 5, 6);
		}
		int s6(int a, int b, int c, int d, int e, int f) {
			return a + b + c + d + e + f;
		}
		"""), 21),
		(("""
		int add(int, int);
		int main() {
			return add(40, 2);
		}
		int add(int a, int b) {
			return a + b;
		}
		"""), 42),
		(("""
		int f(int);
		int f(int y) {
			return y + 1;
		}
		int main() {
			return f(10);
		}
		"""), 11),

	@pytest.mark.parametrize("input", [
		(("""
		int main() {
			return f();
		}
		""")),
		(("""
		int f(int a);
		int f(int a, int b) {
			return a + b;
		}
		int main() {
			return f(1);
		}
		""")),
		(("""
		int f(int a, int b);
		int f(int a, int b) {
			return a + b;
		}
		int main() {
			return f(1);
		}
		""")),
		(("""
		int g(int a);
		int g(int a) {
			return a;
		}
		int main() {
			return g(1, 2);
		}
		""")),
		(("""
		int f(int a);
		int f(int a, int b);
		int main() {
			return 0;
		}
		""")),
		(("""
		int h(int a, int b);
		int main() {
			return h(5);
		}
		""")),
		(("""
		int f(int a);
		int f(int a, int b) {
			return a + b;
		}
		int main() {
			return 1;
		}
		""")),
	])
	def test_forward_decl_fails(self, compile_fail, input):
		assert compile_fail(textwrap.dedent(input)) != 0


	@pytest.mark.parametrize("input, expected", [
		(("""
			int factorial(int n)
			{
				if (n > 1)
					return n * factorial(n - 1);
				
				return n;
			}
			int main()
			{
				return factorial(5);
			}
		"""), 120),
		(("""
			int fib(int n)
			{
				if (n < 2)
					return n;
				return fib(n - 1) + fib(n - 2);
			}
			int main()
			{
				return fib(10);
			}
		"""), 55),
				# linear
		(("""
			int gauss(int n)
			{
				if (n == 0)
					return 0;
				int rest = gauss(n - 1);
				return n + rest;
			}
			int main()
			{
				return gauss(10);
			}
		"""), 55),
				# two arguments
		(("""
			int power(int base, int exp)
			{
				if (exp == 0)
					return 1;
				return base * power(base, exp - 1);
			}
			int main()
			{
				return power(2, 7);
			}
		"""), 128),
		# go 100 levels deep
		(("""
			int depth(int n)
			{
				if (n == 0)
					return 0;
				return 1 + depth(n - 1);
			}
			int main()
			{
				return depth(100);
			}
		"""), 100),
		# recursion through forward declaration
		(("""
			int is_odd(int n);
			int is_even(int n)
			{
				if (n == 0)
					return 1;
				return is_odd(n - 1);
			}
			int is_odd(int n)
			{
				if (n == 0)
					return 0;
				return is_even(n - 1);
			}
			int main()
			{
				return is_even(10);
			}
		"""), 1),
	])
	def test_recursion(self, compile_and_run, input, expected):
		assert compile_and_run(textwrap.dedent(input)) == expected
