#include "bench.hh"

void BenchmarkIntAddInt(B *b) {
	int val = 1;
	int N = b->N;
	for (int i = 0; i < N; i++) {
		NOP(i);
		val += i;
	}
	CONSUME(b,val);
}

void BenchmarkFloatAddInt(B *b) {
    float val = 1;
	int N = b->N;
	for (int i = 0; i < N; i++) {
		NOP(i);
		val += i;
	}
	CONSUME(b,val);
}

void BenchmarkDoubleAddInt(B *b) {
	double val = 1;
	int N = b->N;
	for (int i = 0; i < N; i++) {
		NOP(i);
		val += i;
	}
	CONSUME(b,val);
}

int main() {
	AutoBench(
		Bench(BenchmarkIntAddInt),
		Bench(BenchmarkFloatAddInt),
		Bench(BenchmarkDoubleAddInt),
		);
}
