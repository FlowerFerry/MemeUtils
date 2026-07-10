#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <mmutilspp/thrd/future_watcher.hpp>
#include <future>
#include <thread>
#include <atomic>

using mmupp::thrd::shared_status;
using mmupp::thrd::shared_future_watcher;

// ---------------------------------------------------------------------------
// shared_status
// ---------------------------------------------------------------------------

TEST_CASE("shared_status initial state: not cancelled, progress 0", "[future_watcher]") {
    shared_status s;
    CHECK_FALSE(s.should_cancel());
    CHECK_THAT(s.progress_percent(), Catch::Matchers::WithinAbs(0.0, 1e-9));
}

TEST_CASE("shared_status set_cancel marks cancellation", "[future_watcher]") {
    shared_status s;
    s.set_cancel();
    CHECK(s.should_cancel());
}

TEST_CASE("shared_status set_cancel is idempotent", "[future_watcher]") {
    shared_status s;
    s.set_cancel();
    s.set_cancel();
    CHECK(s.should_cancel());
}

TEST_CASE("shared_status progress_percent returns normalized [0,1] value", "[future_watcher]") {
    shared_status s;
    s.set_progress_range(0, 100);
    s.set_progress(50);
    // (50 - 0) / 100 = 0.5
    CHECK_THAT(s.progress_percent(), Catch::Matchers::WithinAbs(0.5, 1e-9));
}

TEST_CASE("shared_status set_progress overwrites current progress", "[future_watcher]") {
    shared_status s;
    s.set_progress_range(0, 100);
    s.set_progress(30);
    s.set_progress(70);
    // (70 - 0) / 100 = 0.7
    CHECK_THAT(s.progress_percent(), Catch::Matchers::WithinAbs(0.7, 1e-9));
}

TEST_CASE("shared_status increment_progress accumulates correctly", "[future_watcher]") {
    shared_status s;
    s.set_progress_range(0, 100);
    s.set_progress(10);
    s.increment_progress(5);
    s.increment_progress(5);
    // current = 20. (20 - 0) / 100 = 0.2
    CHECK_THAT(s.progress_percent(), Catch::Matchers::WithinAbs(0.2, 1e-9));
}

TEST_CASE("shared_status increment_progress uses default step of 1", "[future_watcher]") {
    shared_status s;
    s.set_progress_range(0, 100);
    s.set_progress(0);
    s.increment_progress();
    s.increment_progress();
    // current = 2. (2 - 0) / 100 = 0.02
    CHECK_THAT(s.progress_percent(), Catch::Matchers::WithinAbs(0.02, 1e-9));
}

TEST_CASE("shared_status inverted progress range is auto-swapped", "[future_watcher]") {
    shared_status s;
    s.set_progress_range(100, 0);  // inverted, should be swapped to min=0, max=100
    s.set_progress(50);
    // After swap: min=0, max=100. (50 - 0) / 100 = 0.5
    CHECK_THAT(s.progress_percent(), Catch::Matchers::WithinAbs(0.5, 1e-9));
}

TEST_CASE("shared_status zero-range reports 0.0", "[future_watcher]") {
    // Range of 0 triggers the early-return 0.0 path
    shared_status s;
    s.set_progress_range(5, 5);
    s.set_progress(5);
    CHECK_THAT(s.progress_percent(), Catch::Matchers::WithinAbs(0.0, 1e-9));
}

// ---------------------------------------------------------------------------
// shared_future_watcher<int>
// ---------------------------------------------------------------------------

TEST_CASE("shared_future_watcher is_ready returns false before fulfillment", "[future_watcher]") {
    shared_future_watcher<int> watcher;
    std::promise<int> p;
    watcher.set_future(p.get_future().share());
    CHECK_FALSE(watcher.is_ready());
}

TEST_CASE("shared_future_watcher is_ready returns true after fulfillment", "[future_watcher]") {
    shared_future_watcher<int> watcher;
    std::promise<int> p;
    watcher.set_future(p.get_future().share());
    p.set_value(42);
    CHECK(watcher.is_ready());
}

TEST_CASE("shared_future_watcher get_result returns the promised value", "[future_watcher]") {
    shared_future_watcher<int> watcher;
    std::promise<int> p;
    watcher.set_future(p.get_future().share());
    p.set_value(1234);
    CHECK(watcher.get_result() == 1234);
}

TEST_CASE("shared_future_watcher cancel flag initially false", "[future_watcher]") {
    shared_future_watcher<int> watcher;
    CHECK_FALSE(watcher.is_cancelled());
}

TEST_CASE("shared_future_watcher set_cancel marks as cancelled", "[future_watcher]") {
    shared_future_watcher<int> watcher;
    watcher.set_cancel();
    CHECK(watcher.is_cancelled());
}

TEST_CASE("shared_future_watcher get_status returns non-null shared_status", "[future_watcher]") {
    shared_future_watcher<int> watcher;
    CHECK(watcher.get_status() != nullptr);
}

TEST_CASE("shared_future_watcher cancel propagates through get_status", "[future_watcher]") {
    shared_future_watcher<int> watcher;
    auto status = watcher.get_status();
    status->set_cancel();
    CHECK(watcher.is_cancelled());
}

// ---------------------------------------------------------------------------
// shared_future_watcher::wait_for — conditional overload
// ---------------------------------------------------------------------------

TEST_CASE("wait_for condition returns true interrupts immediately", "[future_watcher]") {
    shared_future_watcher<int> watcher;
    std::promise<int> p;
    watcher.set_future(p.get_future().share());  // never fulfilled

    auto status = watcher.wait_for(std::chrono::milliseconds(500),
                                   []() { return true; });
    // condition broke the loop; final zero-wait_for reports timeout
    CHECK(status == std::future_status::timeout);
}

TEST_CASE("wait_for future fulfilled during condition poll returns ready", "[future_watcher]") {
    shared_future_watcher<int> watcher;
    std::promise<int> p;
    watcher.set_future(p.get_future().share());

    std::thread t([&p]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        p.set_value(7);
    });

    auto status = watcher.wait_for(std::chrono::seconds(5),
                                   []() { return false; });
    t.join();
    CHECK(status == std::future_status::ready);
    CHECK(watcher.get_result() == 7);
}

TEST_CASE("wait_for condition flips true mid-wait interrupts early", "[future_watcher]") {
    shared_future_watcher<int> watcher;
    std::promise<int> p;
    watcher.set_future(p.get_future().share());  // never fulfilled

    std::atomic<bool> flag{false};
    std::thread t([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        flag = true;
    });

    auto t0 = std::chrono::steady_clock::now();
    auto status = watcher.wait_for(std::chrono::seconds(10),
                                   [&]() { return flag.load(); });
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0).count();
    t.join();

    CHECK(status == std::future_status::timeout);
    CHECK(elapsed >= 80);
    CHECK(elapsed < 2000);
}

TEST_CASE("wait_for full timeout without condition returns timeout", "[future_watcher]") {
    shared_future_watcher<int> watcher;
    std::promise<int> p;
    watcher.set_future(p.get_future().share());  // never fulfilled

    auto t0 = std::chrono::steady_clock::now();
    auto status = watcher.wait_for(std::chrono::milliseconds(100),
                                   []() { return false; });
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0).count();

    CHECK(status == std::future_status::timeout);
    CHECK(elapsed >= 80);
    CHECK(elapsed < 500);
}

TEST_CASE("wait_for no future returns ready immediately", "[future_watcher]") {
    shared_future_watcher<int> watcher;
    auto status = watcher.wait_for(std::chrono::seconds(10),
                                   []() { return false; });
    CHECK(status == std::future_status::ready);
}

TEST_CASE("wait_for condition uses shared_status::should_cancel", "[future_watcher]") {
    shared_future_watcher<int> watcher;
    std::promise<int> p;
    watcher.set_future(p.get_future().share());  // never fulfilled

    std::thread t([&watcher]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        watcher.set_cancel();
    });

    auto status = watcher.wait_for(std::chrono::seconds(10),
                                   [&]() { return watcher.should_cancel(); });
    t.join();

    CHECK(watcher.is_cancelled());
    CHECK(status == std::future_status::timeout);
}

// ---------------------------------------------------------------------------
// shared_future_watcher::wait_until — conditional overload
// ---------------------------------------------------------------------------

TEST_CASE("wait_until condition returns true interrupts immediately", "[future_watcher]") {
    shared_future_watcher<int> watcher;
    std::promise<int> p;
    watcher.set_future(p.get_future().share());

    auto status = watcher.wait_until(
        std::chrono::steady_clock::now() + std::chrono::seconds(2),
        []() { return true; });
    CHECK(status == std::future_status::timeout);
}

TEST_CASE("wait_until future fulfilled during condition poll returns ready", "[future_watcher]") {
    shared_future_watcher<int> watcher;
    std::promise<int> p;
    watcher.set_future(p.get_future().share());

    std::thread t([&p]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        p.set_value(42);
    });

    auto status = watcher.wait_until(
        std::chrono::steady_clock::now() + std::chrono::seconds(5),
        []() { return false; });
    t.join();
    CHECK(status == std::future_status::ready);
    CHECK(watcher.get_result() == 42);
}

TEST_CASE("wait_until condition flips true mid-wait interrupts early", "[future_watcher]") {
    shared_future_watcher<int> watcher;
    std::promise<int> p;
    watcher.set_future(p.get_future().share());

    std::atomic<bool> flag{false};
    std::thread t([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        flag = true;
    });

    auto t0 = std::chrono::steady_clock::now();
    auto status = watcher.wait_until(
        std::chrono::steady_clock::now() + std::chrono::seconds(10),
        [&]() { return flag.load(); });
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0).count();
    t.join();

    CHECK(status == std::future_status::timeout);
    CHECK(elapsed >= 80);
    CHECK(elapsed < 2000);
}

TEST_CASE("wait_until timeout_time reached returns timeout", "[future_watcher]") {
    shared_future_watcher<int> watcher;
    std::promise<int> p;
    watcher.set_future(p.get_future().share());

    auto t0 = std::chrono::steady_clock::now();
    auto status = watcher.wait_until(
        std::chrono::steady_clock::now() + std::chrono::milliseconds(100),
        []() { return false; });
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0).count();

    CHECK(status == std::future_status::timeout);
    CHECK(elapsed >= 80);
    CHECK(elapsed < 500);
}

TEST_CASE("wait_until no future returns ready immediately", "[future_watcher]") {
    shared_future_watcher<int> watcher;
    auto status = watcher.wait_until(
        std::chrono::steady_clock::now() + std::chrono::seconds(10),
        []() { return false; });
    CHECK(status == std::future_status::ready);
}

TEST_CASE("wait_until works with system_clock time point", "[future_watcher]") {
    shared_future_watcher<int> watcher;
    std::promise<int> p;
    watcher.set_future(p.get_future().share());

    auto status = watcher.wait_until(
        std::chrono::system_clock::now() + std::chrono::milliseconds(50),
        []() { return false; });
    CHECK(status == std::future_status::timeout);
}

TEST_CASE("wait_until condition uses shared_status::should_cancel", "[future_watcher]") {
    shared_future_watcher<int> watcher;
    std::promise<int> p;
    watcher.set_future(p.get_future().share());

    std::thread t([&watcher]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        watcher.set_cancel();
    });

    auto status = watcher.wait_until(
        std::chrono::steady_clock::now() + std::chrono::seconds(10),
        [&]() { return watcher.should_cancel(); });
    t.join();

    CHECK(watcher.is_cancelled());
    CHECK(status == std::future_status::timeout);
}
