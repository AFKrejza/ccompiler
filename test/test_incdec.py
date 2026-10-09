import pytest
import textwrap

class Tests:
	@pytest.mark.parametrize("op, expected", [
			("++", [2, 254, 1, 2, 1, 1, 3, 6, 2, 1, 1, 4, 2, 3, 1]),
			("--", [0, 255, 1, 2, 1, 1, 3, 0, 0, 1, 1, 0, 2, 1, 255])
		])
	def test_both(self, compile_and_run, op, expected):
		programs = [
			f"""
			int main() 
			{{
				int a = 1;
				a{op};
				return a;
			}}
			""",
			f"""
			int main()
			{{
				int a = 1;
				int b = 0;
				return b -{op}a;
			}}
			""",
			f"""
			int main()
			{{
				int a = 1;
				return a{op};
			}}
			""",
			f"""
			int main()
			{{
				int a = 1;
				return 1 + a{op};
			}}
			""",
			f"""
			int main()
			{{
				int a = 1;
				a = a{op};
				return a;
			}}
			""",
			f"""
			int main()
			{{
				int a = 1;
				return a = a{op};
			}}
			""",
			f"""
			int main()
			{{
				int a = 1;
				a = a{op} * 3;
				return a;
			}}
			""",
			f"""
			int main()
			{{
				int a = 1;
				a = {op}a * 2 + a{op};
				return a;
			}}
			""",
			f"""
			int main()
			{{
				int a;
				int b;
				int c;
				int d = 1;
				a = b = c = {op}d;
				return a;
			}}
			""",
			f"""
			int main()
			{{
				int a;
				int b;
				int c;
				int d = 1;
				a = b = c = d{op};
				return a;
			}}
			""",
			f"""
			int main()
			{{
				int c;
				int d = 1;
				c = d{op};
				return c;
			}}
			""",
			f"""
			int main()
			{{
				int a = 1;
				a = {op}a * 2;
				return a;
			}}
			""",
			f"""
			int main()
			{{
				int a = 1;
				a = a{op} + 1;
				return a;
			}}
			""",
			f"""
			int main()
			{{
				int a = 1;
				a = a{op} + a{op};
				return a;
			}}
			""",
			f"""
			int main()
			{{
				int a = 1;
				a = {op}a - a{op};
				return a;
			}}
			""",
		]

		for i in range(len(programs)):
			program = textwrap.dedent(programs[i])
			res = compile_and_run(program)
			assert res == expected[i], print(f"{program}Test # {i + 1}: got {res}, expected {expected[i]}")

	@pytest.mark.parametrize("input", [
		"""
		int main()
		{
			++;
		}
		""",
		"""
		int main()
		{
			--;
		}
		""",
		"""
		int main()
		{
			int a = 0;
			++++a;
		}
		""",
		"""
		int main()
		{
			int a = 0;
			----a;
		}
		""",
		"""
		int main()
		{
			int a = 0;
			--a++;
		}
		""",
		"""
		int main()
		{
			int a = 0;
			++a--;
		}
		""",
	])
	def test_fails(self, compile_fail, input):
		program = textwrap.dedent(input)
		assert compile_fail(program) != 0
		# assert result.returncode != 0, f"{program}Compilation succeeded when it shouldn't have: {result.stderr}"
