#include <catch2/catch_test_macros.hpp>
#include <mmutilspp/numeric/value_range.h>
#include <limits>

using mmupp::numeric::value_range;

TEST_CASE("value_range default construction is invalid", "[value_range]") {
    value_range<int> r;
    CHECK(r.min() == 0);
    CHECK(r.max() == 0);
    CHECK(r.invalid());
}

TEST_CASE("value_range normal construction", "[value_range]") {
    value_range<int> r(1, 10);
    CHECK(r.min() == 1);
    CHECK(r.max() == 10);
    CHECK_FALSE(r.invalid());
}

TEST_CASE("value_range inverted construction auto-swaps min and max", "[value_range]") {
    value_range<int> r(10, 1);
    CHECK(r.min() == 1);
    CHECK(r.max() == 10);
    CHECK_FALSE(r.invalid());
}

TEST_CASE("value_range contains: inclusive boundaries and middle values", "[value_range]") {
    value_range<int> r(1, 10);
    CHECK(r.contains(1));
    CHECK(r.contains(5));
    CHECK(r.contains(10));
    CHECK_FALSE(r.contains(0));
    CHECK_FALSE(r.contains(11));
}

TEST_CASE("value_range contains on invalid range always returns true", "[value_range]") {
    value_range<int> r;
    CHECK(r.contains(0));
    CHECK(r.contains(42));
    CHECK(r.contains(-100));
}

TEST_CASE("value_range contains_exclusive excludes boundaries, includes middle", "[value_range]") {
    value_range<int> r(1, 10);
    CHECK_FALSE(r.contains_exclusive(1));
    CHECK(r.contains_exclusive(5));
    CHECK_FALSE(r.contains_exclusive(10));
    CHECK_FALSE(r.contains_exclusive(0));
    CHECK_FALSE(r.contains_exclusive(11));
}

TEST_CASE("value_range contains_exclusive on invalid range always returns true", "[value_range]") {
    value_range<int> r;
    CHECK(r.contains_exclusive(0));
    CHECK(r.contains_exclusive(99));
}

TEST_CASE("value_range set updates both bounds", "[value_range]") {
    value_range<int> r(1, 10);
    r.set(20, 30);
    CHECK(r.min() == 20);
    CHECK(r.max() == 30);
}

TEST_CASE("value_range set with inverted args auto-swaps", "[value_range]") {
    value_range<int> r;
    r.set(50, 10);
    CHECK(r.min() == 10);
    CHECK(r.max() == 50);
}

TEST_CASE("value_range reset makes range invalid", "[value_range]") {
    value_range<int> r(1, 10);
    r.reset();
    CHECK(r.min() == 0);
    CHECK(r.max() == 0);
    CHECK(r.invalid());
}

TEST_CASE("value_range set_min updates lower bound", "[value_range]") {
    value_range<int> r(5, 15);
    r.set_min(2);
    CHECK(r.min() == 2);
    CHECK(r.max() == 15);
}

TEST_CASE("value_range set_max updates upper bound", "[value_range]") {
    value_range<int> r(5, 15);
    r.set_max(25);
    CHECK(r.min() == 5);
    CHECK(r.max() == 25);
}

TEST_CASE("value_range set_min swaps if new min exceeds current max", "[value_range]") {
    value_range<int> r(5, 15);
    r.set_min(20);
    CHECK(r.min() == 15);
    CHECK(r.max() == 20);
}

TEST_CASE("value_range set_max swaps if new max is below current min", "[value_range]") {
    value_range<int> r(10, 20);
    r.set_max(5);
    CHECK(r.min() == 5);
    CHECK(r.max() == 10);
}

TEST_CASE("value_range equality: equal ranges compare equal", "[value_range]") {
    value_range<int> a(1, 10);
    value_range<int> b(1, 10);
    CHECK(a == b);
    CHECK_FALSE(a != b);
}

TEST_CASE("value_range equality: different ranges compare not-equal", "[value_range]") {
    value_range<int> a(1, 10);
    value_range<int> b(2, 10);
    CHECK_FALSE(a == b);
    CHECK(a != b);
}

TEST_CASE("value_range comparison operators compare by range size", "[value_range]") {
    value_range<int> small_(1, 5);  // size 4
    value_range<int> large_(1, 10); // size 9
    CHECK(small_ < large_);
    CHECK(large_ > small_);
    CHECK(small_ <= large_);
    CHECK(large_ >= small_);
    CHECK(small_ <= small_);
    CHECK(small_ >= small_);
    CHECK_FALSE(small_ > large_);
    CHECK_FALSE(large_ < small_);
}

TEST_CASE("value_range operator+ adds corresponding bounds", "[value_range]") {
    value_range<int> a(1, 5);
    value_range<int> b(2, 3);
    auto c = a + b;
    CHECK(c.min() == 3);
    CHECK(c.max() == 8);
}

TEST_CASE("value_range operator- subtracts corresponding bounds", "[value_range]") {
    value_range<int> a(10, 20);
    value_range<int> b(1, 5);
    auto c = a - b;
    CHECK(c.min() == 9);
    CHECK(c.max() == 15);
}

TEST_CASE("value_range operator* chooses correct product min/max", "[value_range]") {
    value_range<int> a(2, 4);
    value_range<int> b(3, 5);
    auto c = a * b;
    CHECK(c.min() == 6);
    CHECK(c.max() == 20);
}

TEST_CASE("value_range operator/ with valid divisor range", "[value_range]") {
    value_range<int> a(10, 20);
    value_range<int> b(2, 4);
    auto c = a / b;
    // min: min(10/2, 20/2, 10/4, 20/4) = min(5,10,2,5) = 2
    // max: max(5,10,2,5) = 10
    CHECK(c.min() == 2);
    CHECK(c.max() == 10);
}

TEST_CASE("value_range operator/ by invalid range returns full extent", "[value_range]") {
    value_range<int> a(1, 10);
    value_range<int> zero;  // invalid (0,0)
    auto c = a / zero;
    CHECK(c.min() == std::numeric_limits<int>::min());
    CHECK(c.max() == std::numeric_limits<int>::max());
}

TEST_CASE("value_range works with double type", "[value_range]") {
    value_range<double> r(1.5, 3.5);
    CHECK(r.min() == 1.5);
    CHECK(r.max() == 3.5);
    CHECK(r.contains(2.0));
    CHECK_FALSE(r.contains(4.0));
}
