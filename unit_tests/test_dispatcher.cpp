#include <catch2/catch_test_macros.hpp>
#include <mmutilspp/thrd/dispatcher.hpp>
#include <atomic>
#include <mutex>
#include <set>
#include <thread>
#include <chrono>
#include <stdexcept>

using mmupp::thrd::dispatcher;

TEST_CASE("dispatcher single thread processes all submitted items", "[dispatcher]") {
    std::atomic<int> count{0};
    {
        dispatcher<int> d(1, [&](int&&) { count.fetch_add(1, std::memory_order_relaxed); });
        for (int i = 0; i < 100; ++i)
            d.submit(i);
        d.wait_for_tasks();
    }
    CHECK(count.load() == 100);
}

TEST_CASE("dispatcher multi-thread processes all submitted items", "[dispatcher]") {
    std::atomic<int> count{0};
    {
        dispatcher<int> d(4, [&](int&&) { count.fetch_add(1, std::memory_order_relaxed); });
        for (int i = 0; i < 1000; ++i)
            d.submit(i);
        d.wait_for_tasks();
    }
    CHECK(count.load() == 1000);
}

TEST_CASE("dispatcher callback receives each submitted value exactly once", "[dispatcher]") {
    std::mutex mtx;
    std::set<int> received;
    {
        dispatcher<int> d(2, [&](int&& val) {
            std::lock_guard<std::mutex> lock(mtx);
            received.insert(val);
        });
        for (int i = 0; i < 50; ++i)
            d.submit(i);
        d.wait_for_tasks();
    }
    REQUIRE(static_cast<int>(received.size()) == 50);
    for (int i = 0; i < 50; ++i)
        CHECK(received.count(i) == 1);
}

TEST_CASE("dispatcher rvalue overload moves submitted item into callback", "[dispatcher]") {
    std::atomic<int> sum{0};
    {
        dispatcher<int> d(1, [&](int&& val) {
            sum.fetch_add(val, std::memory_order_relaxed);
        });
        d.submit(10);
        d.submit(20);
        d.submit(30);
        d.wait_for_tasks();
    }
    CHECK(sum.load() == 60);
}

TEST_CASE("dispatcher determine_thread_count with 0 uses hardware concurrency or 1", "[dispatcher]") {
    auto count = dispatcher<int>::determine_thread_count(0);
    auto hw = std::thread::hardware_concurrency();
    if (hw > 0)
        CHECK(count == hw);
    else
        CHECK(count == 1);
}

TEST_CASE("dispatcher determine_thread_count respects explicit positive value", "[dispatcher]") {
    CHECK(dispatcher<int>::determine_thread_count(1) == 1);
    CHECK(dispatcher<int>::determine_thread_count(4) == 4);
    CHECK(dispatcher<int>::determine_thread_count(8) == 8);
}

TEST_CASE("dispatcher reset with new thread count continues processing", "[dispatcher]") {
    std::atomic<int> count{0};
    dispatcher<int> d(2, [&](int&&) { count.fetch_add(1, std::memory_order_relaxed); });

    for (int i = 0; i < 50; ++i)
        d.submit(i);
    d.wait_for_tasks();
    CHECK(count.load() == 50);

    d.reset(3);
    for (int i = 0; i < 30; ++i)
        d.submit(i);
    d.wait_for_tasks();
    CHECK(count.load() == 80);
}

TEST_CASE("dispatcher destructor waits for all in-flight tasks", "[dispatcher]") {
    std::atomic<int> count{0};
    {
        dispatcher<int> d(2, [&](int&&) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            count.fetch_add(1, std::memory_order_relaxed);
        });
        for (int i = 0; i < 10; ++i)
            d.submit(i);
        // destructor is called here, which calls wait_for_tasks internally
    }
    CHECK(count.load() == 10);
}

TEST_CASE("dispatcher submit lvalue overload copies item into queue", "[dispatcher]") {
    std::atomic<int> sum{0};
    {
        dispatcher<int> d(1, [&](int&& val) {
            sum.fetch_add(val, std::memory_order_relaxed);
        });
        int a = 7, b = 35;
        d.submit(a);   // const& overload
        d.submit(b);   // const& overload
        d.wait_for_tasks();
    }
    CHECK(sum.load() == 42);
}

TEST_CASE("dispatcher reset with default argument keeps hardware thread count", "[dispatcher]") {
    std::atomic<int> count{0};
    {
        dispatcher<int> d(2, [&](int&&) { count.fetch_add(1, std::memory_order_relaxed); });
        d.reset();  // 0 -> hardware concurrency (or 1)
        for (int i = 0; i < 20; ++i)
            d.submit(i);
        d.wait_for_tasks();
    }
    CHECK(count.load() == 20);
}

TEST_CASE("dispatcher callback exception is swallowed and queue keeps draining", "[dispatcher]") {
    std::atomic<int> seen{0};
    std::atomic<int> thrown{0};
    {
        dispatcher<int> d(1, [&](int&& val) {
            seen.fetch_add(1, std::memory_order_relaxed);
            if (val % 2 == 0) {
                thrown.fetch_add(1, std::memory_order_relaxed);
                throw std::runtime_error("boom");
            }
        });
        for (int i = 0; i < 10; ++i)
            d.submit(i);
        d.wait_for_tasks();
        // must not hang or terminate: all 10 items were dequeued
    }
    CHECK(seen.load() == 10);
    CHECK(thrown.load() == 5);
}

TEST_CASE("dispatcher wait_for_tasks returns immediately on empty queue", "[dispatcher]") {
    dispatcher<int> d(2, [](int&&) {});
    auto t0 = std::chrono::steady_clock::now();
    d.wait_for_tasks();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0).count();
    CHECK(elapsed < 500);
}
