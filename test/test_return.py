import pytest

from constants import u_max, i_max

# Test returning from main.
class TestReturn:

	@pytest.mark.parametrize("input, expected", [
		(0, 0),
		(1, 1),
		(u_max(8), u_max(8)),
		(256, 0) # uint8
	])
	def test_literal_exit_code(self, compile_and_run, input, expected):
		source = f'int main() {{ return {input}; }}'
		assert compile_and_run(source) == expected


	def test_empty_return(self, compile_fail):
		source = "int main () { return ; }"
		assert compile_fail(source) != 0

	@pytest.mark.parametrize("input, expected", [
		("int main() { return 1 + 1; }", 2),
		("int main() { return 0 + 0; }", 0),
		("int main() { return 254 + 1; }", 255),
		("int main() { return 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10; }", 55),
	])
	def test_add(self, compile_and_run, input, expected):
		assert compile_and_run(input) == expected

	@pytest.mark.parametrize("input, expected", [
		("int main() { return 1 - 1; }", 0),
		("int main() { return 0 - 0; }", 0),
		(f"int main() {{ return {u_max(32) - u_max(32) + 1} ; }}", 1),
		("int main() { return 254 + 1; }", 255),
		("int main() { return 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10; }", 55),
	])
	def test_sub(self, compile_and_run, input, expected):
		assert compile_and_run(input) == expected

	def test_add_sub(self, compile_and_run):
		source = "int main() { return 1 - 2 + 3; }"
		assert compile_and_run(source) == 2

	def test_add_sub_megalodon(self, compile_and_run):
		# tested at 52,337 operations but it's unstable,
		# more causes segfaults and I haven't looked into why.
		# 20,001 operations worked.

		megalodon = "int main() { return 1"
		for i in range(1000):
			megalodon = megalodon + " - 1 + 1"
		megalodon = megalodon + ";}"
		assert compile_and_run(megalodon) == 1
