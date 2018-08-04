#include "bench.hh"

void BenchmarkIntAddInt(B *b) {
	int val = 1;
	int N = b->N;
	for (int i = 0; i < N; i++) {
		val += i;
		NOPW(val);
	}
}

void BenchmarkFloatAddInt(B *b) {
	float val = 1;
	int N = b->N;
	for (int i = 0; i < N; i++) {
		val += i;
		NOPW(val);
	}
}

void BenchmarkDoubleAddInt(B *b) {
	double val = 1;
	int N = b->N;
	for (int i = 0; i < N; i++) {
		val += i;
		NOPF(val);
	}
}

int main() {
	AutoBench(
		Bench(BenchmarkIntAddInt),
		Bench(BenchmarkFloatAddInt),
		Bench(BenchmarkDoubleAddInt),
		);
}
