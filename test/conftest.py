import os
import pytest
import subprocess
import tempfile

@pytest.fixture
def compile_and_run():
	created = []

	def _run(source):
		with tempfile.NamedTemporaryFile(mode='w', suffix='.c', delete=False) as f:
			f.write(source)
			path = f.name
		created.append(path)

		result = subprocess.run(['./main', path], capture_output=True, text=True)
		assert result.returncode == 0, f"Compilation failed: {result.stderr}"

		run = subprocess.run(['./out'])
		return run.returncode

	yield _run

	for path in created:
		os.unlink(path)


@pytest.fixture
def compile_fail():
	created = []

	def _run(source):
		with tempfile.NamedTemporaryFile(mode='w', suffix='.c', delete=False) as f:
			f.write(source)
			path = f.name
		created.append(path)

		result = subprocess.run(['./main', path], capture_output=True, text=True)
		assert result.returncode != 0, f"Compilation succeeded when it shouldn't have: {result.stderr}"

		return result.returncode

	yield _run

	for path in created:
		os.unlink(path)