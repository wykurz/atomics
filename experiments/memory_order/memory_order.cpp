#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <benchmark/benchmark.h>

constexpr std::size_t SIZE = 1024;

template<std::memory_order LoadOrder, std::memory_order StoreOrder>
void loadStore(benchmark::State& state_)
{
    std::array<std::atomic<std::int64_t>, SIZE> array;
    while (state_.KeepRunning()) {
        state_.PauseTiming();
        std::fill(array.begin(), array.end(), 0);
        state_.ResumeTiming();
        for (int i = 0; i < state_.range_x(); ++i)
        {
            array[i].store(i, StoreOrder);
        }
        std::int64_t sum = 0;
        for (int i = 0; i < state_.range_x(); ++i)
        {
            sum += array[i].load(LoadOrder);
        }
    }
}

BENCHMARK_TEMPLATE(loadStore, std::memory_order_acquire, std::memory_order_release)->Range(1, SIZE);

BENCHMARK_MAIN();
