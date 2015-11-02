#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cstdint>
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
        // static assert Size is power of 2
        std::fill(array.begin(), array.end(), 0);
    }

    void store(std::size_t index_, Type value_)
    {
        array[index_ & Size].store(value_, StoreOrder);
    }

    Type load(std::size_t index_)
    {
        return array[index_ & Size].load(LoadOrder);
    }

  private:
    std::array<std::atomic<Type>, SIZE> array;
};

using AcquireReleaseFixture = StoreLoadFixture<std::int64_t,
                                               SIZE,
                                               std::memory_order_acquire,
                                               std::memory_order_release>;

BENCHMARK_DEFINE_F(AcquireReleaseFixture, LoadStoreTest)(benchmark::State& state_)
{
    if (state_.thread_index == 0) {
        while (state_.KeepRunning())
            for (int i = 0; i < state_.range_x(); ++i)
                store(i, i);
    }
    else {
        while (state_.KeepRunning())
        {
            std::int64_t sum = 0;
            for (int i = 0; i < state_.range_x(); ++i)
                sum += load(i);
        }
    }
}

BENCHMARK_REGISTER_F(AcquireReleaseFixture, LoadStoreTest)->Threads(2)->Range(1 << 10, 1 << 20);

BENCHMARK_MAIN();
