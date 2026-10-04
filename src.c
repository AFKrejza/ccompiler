int max(int a, int b) {
	if (a > b)
		return a;
	else
		return b;
}
int min(int a, int b) {
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
