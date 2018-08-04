# cppbench - define C++ benchmarks like in Go

**cppbench** is a small benchmarking library for C++ that mimics Go's
`go test -bench`. It uses the same benchmarking methodology and thus
can be used to compare C++ and Go benchmarks side-by-side and obtain
meaningful comparisons.

To compile the included example program run:

    $ c++ -std=c++17 -O3 -o bench bench.cc example.cc
    $ ./bench

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

**Beware!** C++ compilers tend to be agressive at eliminating dead
code and folding loops, much more than the Go compiler. To ensure that
the benchmark loops runs for all N iterations, peruse the provided
pseudo-functions `NOP(i)` (inside the loop, on the loop counter) and
`CONSUME()` (at the end, for computer results). See the example usage
in `example.cc`.

The following functionality are supported:

- configuring the number of iterations of the benchmark with the
  global variable `g_count`.
- configuring the "chattiness" of the benchmark execution with
  the global variable `g_chatty`.
- configuring the benchmark time per iteration with the global
  variable `g_benchTime`.
- starting, stopping and resetting the timer with `b->StartTimer()`.
  `b->StopTimer()` and `b->ResetTimer()`.
- running a sub-benchmark with `b->Run()`.

(For the 3 global variables, either set them directly from your
`main()` function, or you will need to define your own command-lin
argument handling to customize them on every run.)

The following functionality from Go's benchmarking library are not supported:

- filtering which benchmarks to run (this may be added in the future).
- `b.Skip()`, `b.Error()`, `b.Fatal()` (this may be added in the future).
- timeouts.
- benchmarking memory usage.
- benchmarking allocations.
- `b.Parallel()`.
- other functionality from Go's `testing` package.

Happy benchmarking!
