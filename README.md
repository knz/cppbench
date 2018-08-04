# cppbench - define C++ benchmarks like in Go

**cppbench** is a small benchmarking library for C++ that mimics Go's
`go test -bench`. It uses the same benchmarking methodology and thus
can be used to compare C++ and Go benchmarks side-by-side and obtain
meaningful comparisons.

To compile the included example program run:

   $ c++ -O3 -o bench bench.cc example.cc
   $ ./a.out

To define your own benchmark function use the following model,
analogous to Go's benchmarking facility:

```c++
void BenchmarkFoo(B *b) {
	int N = b->N;
	for (int i = 0; i < N; i++) {
	   // ... work you need to benchmark ...
	}
}

int main() {
   AutoBench(
	  Bench(BenchmarkFoo),
   );
}
```

Then compile your program with the enclosed `bench.hh` and `bench.cc`.

The following functionality are supported:

- starting, stopping and resetting the timer with `b->StartTimer()`.
  `b->StopTimer()` and `b->ResetTimer()`.
- running a sub-benchmark with `b->Run()`.

The following functionality from Go's benchmarking library are not supported:

- filtering which benchmarks to run (this may be added in the future).
- `b.Skip()`, `b.Error()`, `b.Fatal()` (this may be added in the future).
- timeouts.
- benchmarking memory usage.
- benchmarking allocations.
- `b.Parallel()`.
- other functionality from Go's `testing` package.
