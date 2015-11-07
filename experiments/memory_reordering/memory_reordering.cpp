#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <numeric>
#include <thread>

constexpr std::size_t NUM_THREADS = 2;

using TestType = std::size_t;

static std::array<std::atomic<TestType>, NUM_THREADS> mem;
static std::array<std::atomic<TestType>, NUM_THREADS> ret;

template<std::size_t ThreadId,
         std::memory_order LoadOrder,
         std::memory_order StoreOrder>
struct StoreLoad
{
    StoreLoad()
    {
        static_assert(2 == NUM_THREADS,
                      "StoreLoad test requires exactly 2 threads.");
        static_assert(ThreadId < NUM_THREADS,
                      "ThreadId must be less than total threads.");
    }

    void run()
    {
        // Reordering means both functions return 0
        delay();
        mem[x].store(1, StoreOrder);
        asm volatile("" ::: "memory");
        const auto r = mem[y].load(LoadOrder);
        ret[x].store(r, StoreOrder);
    }

    void check()
    {
        if (not isControl())
            return;
        if (ret[x] == 0 and ret[y] == 0)
            ++reordered;
        ++checks;
    }

    void reset()
    {
        if (not isControl())
            return;
        std::fill(mem.begin(), mem.end(), 0);
    }

    void print() const
    {
        if (not isControl())
            return;
        std::cerr << "Reordered:   " << reordered << std::endl;
        std::cerr << "Checks:      " << checks << std::endl;
        std::cerr << "Wait cycles: " << waitCycles << std::endl;
    }

  private:
    void delay()
    {
        constexpr std::size_t maxSpins = 8;
        std::size_t spins = ((ThreadId + 2) * checks) % maxSpins;
        while (0 < spins--) { ++waitCycles; };
    }

    bool isControl() const
    {
        return 0 == ThreadId;
    }

    static constexpr auto x = ThreadId;
    static constexpr auto y = (ThreadId + 1) & NUM_THREADS;

    std::size_t reordered = 0;
    std::size_t checks = 0;
    std::size_t waitCycles = 0;
};

static std::atomic<bool> done(false);

bool synchronize(const std::size_t round)
{
    static std::atomic<std::size_t> sync(0);
    ++sync;
    while (not done and sync < round * NUM_THREADS) {}
    return done;
}

template<typename Test>
void run()
{
    Test test;
    std::size_t round = 0;
    while (not done)
    {
        if (synchronize(++round)) break;
        test.reset();
        if (synchronize(++round)) break;
        test.run();
        if (synchronize(++round)) break;
        test.check();
    }
    test.print();
}

int main()
{
    constexpr auto loadOrder = std::memory_order_relaxed;
    constexpr auto storeOrder = std::memory_order_relaxed;
    std::thread t1(run<StoreLoad<0, loadOrder, storeOrder>>);
    std::thread t2(run<StoreLoad<1, loadOrder, storeOrder>>);
    std::this_thread::sleep_for(std::chrono::seconds(3));
    done = true;
    t1.join();
    t2.join();
    return 0;
}
