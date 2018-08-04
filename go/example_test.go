package main

import "testing"

// This is the "equivalent" benchmark to example.go.
// Run with:
//   go test -bench . -cpu 1

func BenchmarkIntAddInt(b *testing.B) {
	val := 1
	for i := 0; i < b.N; i++ {
		// Go is not yet clever enough to optimize this loop away. If/when
		// it becomes clever, this needs to be extended to clobber i here.
		val += i
	}
	CONSUME(b, val)
}

func BenchmarkFloatAddInt(b *testing.B) {
	val := float32(1)
	for i := 0; i < b.N; i++ {
		val += float32(i)
	}
	CONSUME(b, val)
}

func BenchmarkDoubleAddInt(b *testing.B) {
	val := float64(1)
	for i := 0; i < b.N; i++ {
		val += float64(i)
	}
	CONSUME(b, val)
}

//go:noinline
func CONSUME(b *testing.B, _ ...interface{}) { b.StopTimer() }
