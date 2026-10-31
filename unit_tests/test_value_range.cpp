#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_tostring.hpp>
#include <catch2/catch_approx.hpp>
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

TEST_CASE("value_range operator- uses interval subtraction", "[value_range]") {
    value_range<int> a(10, 20);
    value_range<int> b(1, 5);
    auto c = a - b;
    // interval subtraction: [a.min - b.max, a.max - b.min] = [5, 19]
    CHECK(c.min() == 5);
    CHECK(c.max() == 19);
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

TEST_CASE("value_range cross-type set copies bounds from other range type", "[value_range]") {
    value_range<int> r;
    value_range<double> src(1.7, 9.2);
    r.set(src);
    CHECK(r.min() == 1);
    CHECK(r.max() == 9);
}

TEST_CASE("value_range cross-type set swaps if source bounds are inverted", "[value_range]") {
    // source itself is normalized, so target receives ordered bounds
    value_range<int> r(50, 60);
    value_range<double> src(9.0, 2.0);  // constructor swaps -> (2,9)
    r.set(src);
    CHECK(r.min() == 2);
    CHECK(r.max() == 9);
}

TEST_CASE("value_range copy assignment copies bounds", "[value_range]") {
    value_range<int> a(3, 7);
    value_range<int> b;
    b = a;
    CHECK(b.min() == 3);
    CHECK(b.max() == 7);
    CHECK_FALSE(b.invalid());
}

TEST_CASE("value_range set_min on invalid range fills max with numeric_limits::max", "[value_range]") {
    value_range<int> r;
    r.set_min(5);
    CHECK(r.min() == 5);
    CHECK(r.max() == std::numeric_limits<int>::max());
    CHECK_FALSE(r.invalid());
}

TEST_CASE("value_range set_max on invalid int range fills min with numeric_limits::min", "[value_range]") {
    value_range<int> r;
    r.set_max(5);
    CHECK(r.min() == std::numeric_limits<int>::min());
    CHECK(r.max() == 5);
    CHECK_FALSE(r.invalid());
}

TEST_CASE("value_range set_max on invalid double range fills min with lowest", "[value_range]") {
    value_range<double> r;
    r.set_max(5.0);
    CHECK(r.min() == std::numeric_limits<double>::lowest());
    CHECK(r.max() == 5.0);
}

TEST_CASE("value_range set_min on invalid range then swap when min above filled max", "[value_range]") {
    // after filling max=INT_MAX, setting a huge min keeps order
    value_range<int> r;
    r.set_min(std::numeric_limits<int>::max());
    CHECK(r.min() == std::numeric_limits<int>::max());
}

TEST_CASE("value_range operator* across zero picks correct corners", "[value_range]") {
    value_range<int> a(-2, 3);
    value_range<int> b(-4, 5);
    auto c = a * b;
    // corners: (-2)(-4)=8, (3)(-4)=-12, (-2)(5)=-10, (3)(5)=15
    CHECK(c.min() == -12);
    CHECK(c.max() == 15);
}

TEST_CASE("value_range operator* with one negative range", "[value_range]") {
    value_range<int> a(2, 4);
    value_range<int> b(-3, -1);
    auto c = a * b;
    // corners: -2, -6, -12, -4
    CHECK(c.min() == -12);
    CHECK(c.max() == -2);
}

TEST_CASE("value_range operator/ across zero", "[value_range]") {
    value_range<int> a(-10, 10);
    value_range<int> b(-2, 2);
    auto c = a / b;
    // corners: -10/-2=5, 10/-2=-5, -10/2=-5, 10/2=5
    CHECK(c.min() == -5);
    CHECK(c.max() == 5);
}

TEST_CASE("value_range operator+ with negative ranges", "[value_range]") {
    value_range<int> a(-5, -1);
    value_range<int> b(-3, 2);
    auto c = a + b;
    CHECK(c.min() == -8);
    CHECK(c.max() == 1);
}

TEST_CASE("value_range operator- with negative ranges", "[value_range]") {
    value_range<int> a(-5, -1);
    value_range<int> b(-3, 2);
    auto c = a - b;
    CHECK(c.min() == -7);
    CHECK(c.max() == 2);
}

TEST_CASE("value_range double division yields fractional bounds", "[value_range]") {
    value_range<double> a(1.0, 2.0);
    value_range<double> b(2.0, 4.0);
    auto c = a / b;
    CHECK(c.min() == Catch::Approx(0.25));
    CHECK(c.max() == Catch::Approx(1.0));
}

TEST_CASE("value_range range_size uses unsigned diff for int extremes", "[value_range]") {
    value_range<int> wide(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    value_range<int> small_(0, 1);
    CHECK(small_ < wide);
}
