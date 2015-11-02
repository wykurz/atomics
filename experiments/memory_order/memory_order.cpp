#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <benchmark/benchmark.h>

constexpr std::size_t SIZE = 1 << 10;

template<typename Type,
         std::size_t Size,
         std::memory_order LoadOrder,
         std::memory_order StoreOrder>
struct StoreLoadFixture: public ::benchmark::Fixture
{
    void SetUp()
    {
        static_assert(0 == ((Size - 1) & Size), "Template size agrument must be a power of 2.");
        _dummy = 0;
        std::fill(_array.begin(), _array.end(), 1);
    }

    void storeTest(benchmark::State& state_)
    {
        while (state_.KeepRunning())
            for (int i = 0; i < state_.range_x(); ++i)
                store(i, i);
    }

    void loadTest(benchmark::State& state_)
    {
        while (state_.KeepRunning())
        {
            for (int i = 0; i < state_.range_x(); ++i)
                _dummy += load(i);
        }
    }

    void storeLoadTest(benchmark::State& state_)
    {
        while (state_.KeepRunning())
        {
            for (int i = 0; i < state_.range_x(); ++i)
            {
                store(_dummy & Size, i);
                _dummy += load(i);
            }
        }
    }

    void store(std::size_t index_, Type value_)
    {
        _array[index_ & Size].store(value_, StoreOrder);
    }

    Type load(std::size_t index_)
    {
        return _array[index_ & Size].load(LoadOrder);
    }

  private:
    std::int64_t _dummy;
    std::array<std::atomic<Type>, Size> _array;
};

using SeqCstFixture         = StoreLoadFixture<std::int64_t, SIZE, std::memory_order_seq_cst, std::memory_order_seq_cst>;
using AcquireReleaseFixture = StoreLoadFixture<std::int64_t, SIZE, std::memory_order_acquire, std::memory_order_release>;
using ConsumeReleaseFixture = StoreLoadFixture<std::int64_t, SIZE, std::memory_order_consume, std::memory_order_release>;
using RelaxedFixture        = StoreLoadFixture<std::int64_t, SIZE, std::memory_order_relaxed, std::memory_order_relaxed>;

#define DEFINES(FixtureType) \
    BENCHMARK_DEFINE_F(FixtureType,   StoreTest    )(benchmark::State& state_) { storeTest(    state_); } \
    BENCHMARK_DEFINE_F(FixtureType,   LoadTest     )(benchmark::State& state_) { loadTest(     state_); } \
    BENCHMARK_DEFINE_F(FixtureType,   StoreLoadTest)(benchmark::State& state_) { storeLoadTest(state_); } \
    BENCHMARK_REGISTER_F(FixtureType, StoreTest    )->Range(1 << 20, 1 << 20); \
    BENCHMARK_REGISTER_F(FixtureType, LoadTest     )->Range(1 << 20, 1 << 20); \
    BENCHMARK_REGISTER_F(FixtureType, StoreLoadTest)->Range(1 << 20, 1 << 20);

DEFINES(SeqCstFixture);
DEFINES(AcquireReleaseFixture);
DEFINES(ConsumeReleaseFixture);
DEFINES(RelaxedFixture);

BENCHMARK_MAIN();
