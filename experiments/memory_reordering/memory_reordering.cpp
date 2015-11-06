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

using TestType = int;

static std::array<TestType, NUM_THREADS> mem;
static std::array<TestType, NUM_THREADS> ret;

struct StoreLoad
{
    StoreLoad(std::size_t thread_)
        : thread(thread_)
    {
        static_assert(2 == NUM_THREADS, "StoreLoad test requires exactly 2 threads.");
        assert(thread < NUM_THREADS);
    }

    void run()
    {
        TestType& x = mem[thread];
        TestType& y = mem[(thread + 1) & NUM_THREADS];
        // Reordering means both functions return 0
        TestType r = 0;
        delay();
        x = 1;
        asm volatile("" ::: "memory");
        r = y;
        ret[thread] = r;
    }

    void check()
    {
        if (not isControl())
            return;
        reordered += (0 == std::accumulate(ret.begin(), ret.end(), 0)) ? 1 : 0;
        ++checks;
    }

    void reset()
    {
        if (not isControl())
            return;
        std::fill(mem.begin(), mem.end(), 0);
        std::fill(ret.begin(), ret.end(), 0);
    }

    void print() const
    {
        if (not isControl())
            return;
        std::cerr << "Reordered: " << reordered << std::endl;
        std::cerr << "Checks:    " << checks << std::endl;
    }

  private:
    void delay()
    {
        constexpr std::size_t maxSpins = 8;
        std::size_t spins = ((thread + 2) * checks) % maxSpins;
        while (0 < spins--) {};
    }

    bool isControl() const
    {
        return 0 == thread;
    }

    const std::size_t thread;
    std::size_t reordered = 0;
    std::size_t checks = 0;
};

static std::atomic<bool> done(false);

void synchronize(const std::size_t round)
{
    static std::atomic<std::size_t> sync(0);
    ++sync;
    while (not done and sync < round * NUM_THREADS) {}
}

template<typename Test>
void run(Test& test)
{
    std::size_t round = 0;
    while (not done)
    {
        synchronize(++round);
        test.reset();
        synchronize(++round);
        test.run();
        synchronize(++round);
        test.check();
    }
    test.print();
}

int main()
{
    StoreLoad sl0(0);
    StoreLoad sl1(1);
    std::thread t1([&](){ run(sl0); });
    std::thread t2([&](){ run(sl1); });
    std::this_thread::sleep_for(std::chrono::seconds(10));
    done = true;
    t1.join();
    t2.join();
    return 0;
}
