// Copyright (c) 2018, Raphael 'kena' Poss
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//
// * Neither the name of the copyright holders nor the names of the
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

// This code is a crude C++ approximation of the Go code in
// go/src/testing/benchmark.go in the official Go distribution.

#include "bench.hh"
#include <ctime>
#include <algorithm>
#include <sstream>
#include <iomanip>

bool g_chatty = false;
Duration g_benchTime = Duration{1000000000};
int g_count = 1;
using namespace std;
typedef struct timespec Time;

Duration Since(Time t) {
	Time now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	auto delta = 1000000000*(int64_t(now.tv_sec)-int64_t(t.tv_sec))
		+ int64_t(now.tv_nsec)-int64_t(t.tv_nsec);
	return Duration{delta};
}

// benchmarkName returns full name of benchmark including procs suffix.
string benchmarkName(const string& name, int n) {
	if (n != 1) {
		ostringstream os;
		os << name << '=' << n;
		return os.str();
	}
	return name;
}

string BenchmarkResult::String() const {
	auto& r=  *this;
	auto nsop = r.NsPerOp();
	ostringstream ns;
	ns << setw(10) << nsop << " ns/op";
	if (r.N > 0 && nsop < 100) {
		// The format specifiers here make sure that
		// the ones digits line up for all three possible formats.
		if (nsop < 10) {
			ns = ostringstream();
			ns << fixed << setw(13) << setprecision(2) << double(r.T.Nanoseconds())/double(r.N) << " ns/op";
		} else {
			ns = ostringstream();
			ns << fixed << setw(12) << setprecision(1) << double(r.T.Nanoseconds())/double(r.N) << " ns/op";
		}
	}
	ostringstream res;
	res << setw(8) << r.N << '\t' << ns.str();
	return res.str();
}

// StartTimer starts timing a test. This function is called automatically
// before a benchmark starts, but it can also used to resume timing after
// a call to StopTimer.
void B::StartTimer() {
	auto& b = *this;
	if (!b.timerOn) {
		clock_gettime(CLOCK_MONOTONIC, &b.start);
		b.timerOn = true;
	}
}

// StopTimer stops timing a test. This can be used to pause the timer
// while performing complex initialization that you don't
// want to measure.
void B::StopTimer() {
	auto& b = *this;
	if (b.timerOn) {
		b.duration += Since(b.start);
		b.timerOn = false;
	}
}

// ResetTimer zeros the elapsed benchmark time and memory allocation counters.
// It does not affect whether the timer is running.
void B::ResetTimer() {
	auto& b = *this;
	if (b.timerOn) {
		clock_gettime(CLOCK_MONOTONIC, &b.start);
	}
	b.duration = 0;
}

// runN runs a single benchmark for the specified number of iterations.
void B::runN(int n) {
	auto &b = *this;
	b.N = n;
	b.ResetTimer();
	b.StartTimer();
	b.benchFunc(&b);
	b.StopTimer();
}

// run executes the benchmark in a separate goroutine, including all of its
// subbenchmarks. b must not have subbenchmarks.
void B::run() {
	auto& b = *this;
	if (b.context != 0) {
		// Running go test --test.bench
		b.context->processBench(&b); // Must call doBench.
	} else {
		// Running func Benchmark.
		doBench();
	}
}

// run1 runs the first iteration of benchFunc. It returns whether more
// iterations of this benchmarks should be run.
bool B::run1()  {
	auto& b = *this;
	if (auto ctx = b.context; ctx != 0) {
		// Extend maxLen, if needed.
		if (int n = b.name.length() + ctx->extLen + 1; n > ctx->maxLen) {
			ctx->maxLen = n + 8; // Add additional slack to avoid too many jumps in size.
		}
	}
	b.runN(1);
	if (b.failed) {
		fprintf(stderr,"--- FAIL: %s\n", b.name.c_str());
		return false;
	}
	// Only print the output if we know we are not going to proceed.
	// Otherwise it is printed in processBench.
	if (b.hasSub != 0 || b.finished) {
		auto tag = "BENCH";
		if (b.skipped) {
			tag = "SKIP";
		}
		if (b.chatty && (b.finished)) {
			fprintf(stderr,"--- %s: %s\n", tag, b.name.c_str());
		}
		return false;
	}
	return true;
}

BenchmarkResult B::doBench() {
	launch();
	return result;
}

// Run benchmarks f as a subbenchmark with the given name. It reports
// whether there were any failures.
//
// A subbenchmark is like any other benchmark. A benchmark that calls Run at
// least once will not be measured itself and will be called once with N=1.
bool B::Run(const string& name, std::function<void(B*b)> f) {
	auto& b = *this;
	// Since b has subbenchmarks, we will no longer run it as a benchmark itself.
	// Release the lock and acquire it on exit to ensure locks stay paired.
	b.hasSub = 1;
	B sub;
	auto subname = name;
	if (!b.name.empty())
		subname = b.name + "/" + subname;
	sub.name = subname;
	// sub.parent = (common*)&b;
	// sub.level =  b.level + 1;
	sub.chatty = b.chatty;
	sub.benchFunc = f;
	sub.benchTime = b.benchTime;
	sub.context = b.context;
	if (sub.run1()) {
		sub.run();
	}
	b.add(sub.result);
	return !sub.failed;
}

// add simulates running benchmarks in sequence in a single iteration. It is
// used to give some meaningful results in case func Benchmark is used in
// combination with Run.
void B::add(BenchmarkResult other) {
	auto& b = *this;
	auto& r = b.result;
	// The aggregated BenchmarkResults resemble running all subbenchmarks as
	// in sequence in a single benchmark.
	r.N = 1;
	r.T += Duration{other.NsPerOp()};
}

// roundDown10 rounds a number down to the nearest power of 10.
int roundDown10(int n) {
	int tens = 0;
	// tens = floor(log_10(n))
	while (n >= 10) {
		n = n / 10;
		tens++;
	}
	// result = 10^tens
	int result = 1;
	for (int i = 0; i < tens; i++)
		result *= 10;

	return result;
}


// roundUp rounds x up to a number of the form [1eX, 2eX, 3eX, 5eX].
int roundUp(int n) {
	auto base = roundDown10(n);
	if (n <= base)
		return base;
	if (n <= (2 * base))
		return 2 * base;
	if (n <= (3 * base))
		return 3 * base;
	if (n <= (5 * base))
		return 5 * base;
	return 10 * base;
}



// launch launches the benchmark function. It gradually increases the number
// of benchmark iterations until the benchmark runs for the requested benchtime.
// launch is run by the doBench function as a separate goroutine.
// run1 must have been called on b.
void B::launch() {
	auto& b = *this;
	// Run the benchmark for at least the specified amount of time.
	auto d = b.benchTime;
	for (int n = 1; !b.failed && b.duration < d && n < 1000000000; ) {
		auto last = n;
		// Predict required iterations.
		n = int(d.Nanoseconds());
		if (auto nsop = b.nsPerOp(); nsop != 0) {
			n /= int(nsop);
		}
		// Run more iterations than we think we'll need (1.2x).
		// Don't grow too fast in case we had timing errors previously.
		// Be sure to run at least one more than last time.
		n = max(min(n+n/5, 100*last), last+1);
		// Round up to something easy to read.
		n = roundUp(n);
		b.runN(n);
	}
	b.result = BenchmarkResult{b.N, b.duration};
}

bool runBenchmarks(const vector<BDef>& benchmarks)  {
	int    maxprocs = 1;
    benchContext  ctx;
	ctx.extLen = benchmarkName("", maxprocs).length();
	B main;
	main.chatty = g_chatty;
	main.benchFunc = [&benchmarks](B *b) {
						 for (auto Benchmark : benchmarks) {
							 b->Run(Benchmark.Name, Benchmark.F);
						 }
					 };
    main.benchTime = g_benchTime;
    main.context =  &ctx;
	main.runN(1);
	return !main.failed;
}

// An internal function but exported because it is cross-package; part of the implementation
// of the "go test" command.
void RunBenchmarks(const vector<BDef> &benchmarks) {
	runBenchmarks(/*"", matchString,*/ benchmarks);
}

// processBench runs bench b for the configured CPU counts and prints the results.
void benchContext::processBench(B *pb) {
	auto&ctx = *this;
	auto b = *pb;
	int i = 0;
	int procs = 1;
	for (unsigned j = 0; j < g_count; j++) {
		auto  benchName = benchmarkName(b.name, procs);
		fprintf(stderr, "%-*s\t", ctx.maxLen, benchName.c_str());
		// Recompute the running time for all but the first iteration.
		if (i > 0 || j > 0) {
			B nb;
			nb.name =   b.name;
			nb.chatty = b.chatty;
			nb.benchFunc = b.benchFunc;
			nb.benchTime = b.benchTime;
			b = nb;
			nb.run1();
		}
		auto r = b.doBench();
		if (b.failed) {
			// The output could be very long here, but probably isn't.
			// We print it all, regardless, because we don't want to trim the reason
			// the benchmark failed.
			fprintf(stderr, "--- FAIL: %s\n", benchName.c_str());
			continue;
		}
		auto results = r.String();
		fprintf(stderr, "%s\n", results.c_str());
	}
}
