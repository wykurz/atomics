#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <thread>

constexpr std::size_t NUM_THREADS = 2;

int storeLoad(int& x, int& y)
{
    // Reordering means both functions return 0
    int r = 0;
    x = 1;
    r = y;
    return r;
}

int loadStore(int& x, int& y)
{
    // Reordering means both functions return 1
    int r = 1;
    r = y;
    x = 0;
    return r;
}

void synchronize(const std::size_t round)
{
    static std::atomic<std::size_t> sync(0);
    ++sync;
    while (sync < round * NUM_THREADS) {}
}

static std::atomic<bool> done(false);

template<typename Func>
void run(Func func, std::size_t thread)
{
    static std::array<std::size_t, NUM_THREADS> results;
    std::fill(results.begin(), results.end(), 1);
    std::size_t round = 0;
    std::size_t count = 0;
    while (not done)
    {
        synchronize(++round);
        results[thread] = 0;
        synchronize(++round);
        results[thread] = func();
        synchronize(++round);
        if (0 == thread)
        {
            bool reordered = true;
            for (auto& r: results)
            {
                if (r != 0)
                {
                    reordered = false;
                }
                r = 1;
            }
            if (reordered)
            {
                ++count;
            }
        }
    }
    if (0 == thread)
    {
        std::cerr << "Reordered:" << count << std::endl;
        std::cerr << "Rounds:   " << round << std::endl;
    }
}

int main()
{
    int x;
    int y;
    auto f1 = [&x, &y](){ return storeLoad(x, y); };
    auto f2 = [&x, &y](){ return storeLoad(y, x); };
    std::thread t1([f1](){ run(f1, 0); });
    std::thread t2([f2](){ run(f2, 1); });
    std::this_thread::sleep_for(std::chrono::seconds(10));
    done = true;
    t1.join();
    t2.join();
    return 0;
}
