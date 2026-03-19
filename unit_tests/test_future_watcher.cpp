#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <mmutilspp/thrd/future_watcher.hpp>
#include <future>
#include <thread>

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
