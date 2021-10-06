#include <Protocon/Protocon.h>
#include <benchmark/benchmark.h>

static void BenchSimple(benchmark::State& state) {
    for (auto _ : state)
        ;
}
BENCHMARK(BenchSimple);
