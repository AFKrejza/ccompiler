import subprocess
import tempfile
import os
import pytest

# Test returning from main.
class TestReturn:

	@pytest.mark.parametrize("input, expected", [
		(0, 0),
		(1, 1),
		(255, 255),
		(256, 0) # uint8
	])
	def test_literal_exit_code(self, compile_and_run, input, expected):
		source = f'int main() {{ return {input}; }}'
		assert compile_and_run(source) == expected


	def test_empty_return(self, compile_fail):
		source = "int main () { return ; }"
		assert compile_fail(source) != 0




