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

#ifndef bench_h
#define bench_h

#include <ctime>
#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <cstring>

typedef struct timespec Time;

struct Duration {
	int64_t ns;
	int64_t Nanoseconds() const { return ns; };
	bool operator<(const Duration& other) { return ns < other.ns; };
	Duration& operator=(int64_t other) {
		ns = other;
		return *this;
	};
	Duration& operator+=(const Duration& other) {
		ns += other.ns;
		return *this;
	};
	Duration() : ns(0) {};
	Duration(int64_t ns) : ns(ns) {};
};

Duration Since(Time t);

struct BenchmarkResult {
	int         N; // The number of iterations.
	Duration    T; // The total time taken.

	BenchmarkResult() : N(0), T() {}
	BenchmarkResult(int n, const Duration& t) : N(n), T(t) {}

	std::string String() const;

	int64_t NsPerOp() const {
		if (N <= 0) {
			return 0;
		}
		return T.Nanoseconds() / int64_t(N);
	}
};

struct common {
	bool failed;         // Test or benchmark has failed.
	bool skipped;        // Test of benchmark has been skipped.
	bool done;           // Test is finished and all subtests have completed.

	bool chatty;         // A copy of the chatty flag.
	bool finished;       // Test function has completed.
	int32_t hasSub;

	// common  *parent;
	// int level;        // Nesting depth of test or benchmark.
	std::string name;    // Name of test or benchmark.
	Time start;          // Time test or benchmark started
	Duration duration;

	common() { memset(this, 0, sizeof(*this)); }
};

struct B;
struct benchContext {
	// match *matcher

	int maxLen; // The largest recorded benchmark name.
	int extLen; // Maximum extension length.

	void processBench(B *b);
	benchContext() : maxLen(0), extLen(0) {}
};

struct B : common {
	// Public interface.
	int  N;
	void StartTimer();
	void StopTimer();
	void ResetTimer();
	bool Run(const std::string& name, std::function<void(B *b)>);

	// Implementation.
	benchContext    *context;
	std::function<void(B*b)> benchFunc;
	Duration        benchTime;
	bool            timerOn;
	BenchmarkResult result;

	B() { memset(this, 0, sizeof(*this)); };

	int64_t nsPerOp() {
		auto& b = *this;
		if (b.N <= 0) {
			return 0;
		}
		return b.duration.Nanoseconds() / int64_t(b.N);
	}

	void runN(int n);
	void run();
	bool run1();
	BenchmarkResult doBench();
	void add(BenchmarkResult other);
	void launch();
};

// Used to define a set of benchmarks.
struct BDef {
	std::string Name;
	std::function<void(B*b)> F;
};

// Use Bench as follows:
// AutoBench(
//        Bench(foo),
//        Bench(bar),
//        ...
// );
#define Bench(Func) (BDef{ #Func, Func })
#define AutoBench(...) RunBenchmarks(std::vector<BDef>{ __VA_ARGS__ })

// Use NOPW(x) inside of the benchmarking loop to ensure a loop does
// not get optimized away.
#define NOPW(x) __asm__ __volatile__("" :: "r"(x))

// Use NOPF(x) inside of the benchmarking loop to ensure a loop does
// not get optimized away.
#define NOPF(x) __asm__ __volatile__("" :: "rf"(x))

// Runs a set of benchmarks.
void RunBenchmarks(const std::vector<BDef> &benchmarks);

// Global configuration options.
extern bool     g_chatty;
extern Duration g_benchTime;
extern int      g_count;

#endif
