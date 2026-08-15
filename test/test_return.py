import subprocess
import tempfile
import os
import pytest
import sys

U8_MAX = 255
I8_MAX = 127

U16_MAX = 65535
I16_MAX = 32767

U32_MAX = 4294967296
I32_MAX = 2147483647

U64_MAX = 18446744073709551615
I64_MAX = 9223372036854775807

# Test returning from main.
class TestReturn:

	@pytest.mark.parametrize("input, expected", [
		(0, 0),
		(1, 1),
		(U8_MAX, U8_MAX),
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
		(f"int main() {{ return {U32_MAX - U32_MAX + 1} ; }}", 1),
		("int main() { return 254 + 1; }", 255),
		("int main() { return 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10; }", 55),
	])
	def test_sub(self, compile_and_run, input, expected):
		assert compile_and_run(input) == expected

	def test_add_sub(self, compile_and_run):
		source = "int main() { return 1 - 2 + 3; }"
		assert compile_and_run(source) == 2

