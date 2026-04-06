#include <catch2/catch_test_macros.hpp>
#include <mmutilspp/util/span2d.h>

#include <vector>
#include <stdexcept>

using mmupp::util::span2d;
using mmupp::util::axis_info;
using storage_layout = span2d<double>::storage_layout;

// ---------------------------------------------------------------------------
// Default construction
// ---------------------------------------------------------------------------

TEST_CASE("span2d default construction has zero sizes", "[span2d]") {
    span2d<double> s;
    CHECK(s.x_size() == 0);
    CHECK(s.y_size() == 0);
}

TEST_CASE("span2d default construction has coefficient 1.0", "[span2d]") {
    span2d<double> s;
    CHECK(s.coefficient() == 1.0);
}

TEST_CASE("span2d default construction has offset 0", "[span2d]") {
    span2d<int> s;
    CHECK(s.offset() == 0);
}

TEST_CASE("span2d default construction has x_interval 1 and y_interval 1", "[span2d]") {
    span2d<float> s;
    CHECK(s.x_interval() == 1.0f);
    CHECK(s.y_interval() == 1.0f);
}

TEST_CASE("span2d default construction layout is y_major", "[span2d]") {
    span2d<double> s;
    CHECK(s.layout() == storage_layout::y_major);
}

// ---------------------------------------------------------------------------
// Construction with data
// ---------------------------------------------------------------------------

TEST_CASE("span2d construction with matching span size succeeds", "[span2d]") {
    std::vector<int> data(12, 0); // 3 * 4
    nonstd::span<int> sp{ data.data(), data.size() };
    CHECK_NOTHROW(span2d<int>(sp, 3, 4));
}

TEST_CASE("span2d construction with mismatched span size throws invalid_argument", "[span2d]") {
    std::vector<int> data(10);
    nonstd::span<int> sp{ data.data(), data.size() };
    CHECK_THROWS_AS(span2d<int>(sp, 3, 4), std::invalid_argument);
}

TEST_CASE("span2d construction with axis_info sets axis names and units", "[span2d]") {
    std::vector<double> data(6, 0.0); // 2 * 3
    nonstd::span<double> sp{ data.data(), data.size() };
    axis_info xi{ memepp::string{"time"}, memepp::string{"s"} };
    axis_info yi{ memepp::string{"voltage"}, memepp::string{"V"} };
    span2d<double> s(sp, 2, 3, xi, yi);
    CHECK(std::string(s.x_axis().name.data(), s.x_axis().name.size()) == "time");
    CHECK(std::string(s.x_axis().unit.data(), s.x_axis().unit.size()) == "s");
    CHECK(std::string(s.y_axis().name.data(), s.y_axis().name.size()) == "voltage");
    CHECK(std::string(s.y_axis().unit.data(), s.y_axis().unit.size()) == "V");
}

// ---------------------------------------------------------------------------
// at() — y_major layout (default)
// ---------------------------------------------------------------------------

TEST_CASE("span2d at() y_major: data[y * x_size + x]", "[span2d]") {
    // 3 columns (x), 2 rows (y), y_major: stored row-by-row
    // layout: [row0_col0, row0_col1, row0_col2, row1_col0, row1_col1, row1_col2]
    std::vector<int> data = { 10, 20, 30,
                              40, 50, 60 };
    nonstd::span<int> sp{ data.data(), data.size() };
    span2d<int> s(sp, 3, 2); // x_size=3, y_size=2

    // y_major: index = y * x_size + x
    CHECK(s.at(0, 0) == data[0 * 3 + 0]); // 10
    CHECK(s.at(2, 0) == data[0 * 3 + 2]); // 30
    CHECK(s.at(0, 1) == data[1 * 3 + 0]); // 40
    CHECK(s.at(2, 1) == data[1 * 3 + 2]); // 60
}

// ---------------------------------------------------------------------------
// at() — x_major layout
// ---------------------------------------------------------------------------

TEST_CASE("span2d at() x_major: data[x * y_size + y]", "[span2d]") {
    // 2 columns (x), 3 rows (y), x_major: stored column-by-column
    // layout: [col0_row0, col0_row1, col0_row2, col1_row0, col1_row1, col1_row2]
    std::vector<int> data = { 1, 2, 3,
                              4, 5, 6 };
    nonstd::span<int> sp{ data.data(), data.size() };
    span2d<int> s(sp, 2, 3); // x_size=2, y_size=3
    s.set_layout(span2d<int>::storage_layout::x_major);

    // x_major: index = x * y_size + y
    CHECK(s.at(0, 0) == data[0 * 3 + 0]); // 1
    CHECK(s.at(0, 2) == data[0 * 3 + 2]); // 3
    CHECK(s.at(1, 0) == data[1 * 3 + 0]); // 4
    CHECK(s.at(1, 2) == data[1 * 3 + 2]); // 6
}

// ---------------------------------------------------------------------------
// at() — out of range
// ---------------------------------------------------------------------------

TEST_CASE("span2d at() x out-of-range throws out_of_range", "[span2d]") {
    std::vector<double> data(6, 0.0);
    nonstd::span<double> sp{ data.data(), data.size() };
    span2d<double> s(sp, 3, 2);
    CHECK_THROWS_AS(s.at(3, 0), std::out_of_range); // x == x_size
}

TEST_CASE("span2d at() y out-of-range throws out_of_range", "[span2d]") {
    std::vector<double> data(6, 0.0);
    nonstd::span<double> sp{ data.data(), data.size() };
    span2d<double> s(sp, 3, 2);
    CHECK_THROWS_AS(s.at(0, 2), std::out_of_range); // y == y_size
}

// ---------------------------------------------------------------------------
// coefficient and offset
// ---------------------------------------------------------------------------

TEST_CASE("span2d set_coefficient and set_offset are applied in at()", "[span2d]") {
    std::vector<double> data = { 1.0, 2.0, 3.0, 4.0 };
    nonstd::span<double> sp{ data.data(), data.size() };
    span2d<double> s(sp, 2, 2);
    s.set_coefficient(2.5);
    s.set_offset(1.0);

    // y_major: at(0,0) -> data[0]*2.5 + 1.0 = 1.0*2.5+1.0 = 3.5
    CHECK(s.at(0, 0) == 3.5);
    // at(1,1) -> data[3]*2.5 + 1.0 = 4.0*2.5+1.0 = 11.0
    CHECK(s.at(1, 1) == 11.0);
}

// ---------------------------------------------------------------------------
// x_at() and y_at()
// ---------------------------------------------------------------------------

TEST_CASE("span2d x_at() returns x_interval * index", "[span2d]") {
    std::vector<int> data(6, 0);
    nonstd::span<int> sp{ data.data(), data.size() };
    span2d<int> s(sp, 3, 2);

    CHECK(s.x_at(0) == 0);
    CHECK(s.x_at(1) == 1);
    CHECK(s.x_at(2) == 2);
}

TEST_CASE("span2d y_at() returns y_interval * index", "[span2d]") {
    std::vector<int> data(6, 0);
    nonstd::span<int> sp{ data.data(), data.size() };
    span2d<int> s(sp, 3, 2);

    CHECK(s.y_at(0) == 0);
    CHECK(s.y_at(1) == 1);
}

TEST_CASE("span2d set_x_interval affects x_at()", "[span2d]") {
    std::vector<double> data(6, 0.0);
    nonstd::span<double> sp{ data.data(), data.size() };
    span2d<double> s(sp, 3, 2);
    s.set_x_interval(0.5);

    CHECK(s.x_at(0) == 0.0);
    CHECK(s.x_at(1) == 0.5);
    CHECK(s.x_at(2) == 1.0);
}

TEST_CASE("span2d set_y_interval affects y_at()", "[span2d]") {
    std::vector<double> data(6, 0.0);
    nonstd::span<double> sp{ data.data(), data.size() };
    span2d<double> s(sp, 3, 2);
    s.set_y_interval(3.0);

    CHECK(s.y_at(0) == 0.0);
    CHECK(s.y_at(1) == 3.0);
}

TEST_CASE("span2d x_at() out-of-range throws out_of_range", "[span2d]") {
    std::vector<int> data(6, 0);
    nonstd::span<int> sp{ data.data(), data.size() };
    span2d<int> s(sp, 3, 2);
    CHECK_THROWS_AS(s.x_at(3), std::out_of_range);
}

TEST_CASE("span2d y_at() out-of-range throws out_of_range", "[span2d]") {
    std::vector<int> data(6, 0);
    nonstd::span<int> sp{ data.data(), data.size() };
    span2d<int> s(sp, 3, 2);
    CHECK_THROWS_AS(s.y_at(2), std::out_of_range);
}

// ---------------------------------------------------------------------------
// set_layout
// ---------------------------------------------------------------------------

TEST_CASE("span2d set_layout switches between x_major and y_major", "[span2d]") {
    std::vector<int> data = { 1, 2, 3, 4 };
    nonstd::span<int> sp{ data.data(), data.size() };
    // 2x2 grid
    span2d<int> s(sp, 2, 2);

    // default y_major: at(1,0) -> data[0*2 + 1] = 2
    CHECK(s.at(1, 0) == 2);

    s.set_layout(span2d<int>::storage_layout::x_major);
    // x_major: at(1,0) -> data[1*2 + 0] = 3
    CHECK(s.at(1, 0) == 3);
}

// ---------------------------------------------------------------------------
// set_x_axis / set_y_axis
// ---------------------------------------------------------------------------

TEST_CASE("span2d set_x_axis updates axis info", "[span2d]") {
    span2d<double> s;
    axis_info xi{ memepp::string{"freq"}, memepp::string{"Hz"} };
    s.set_x_axis(xi);
    CHECK(std::string(s.x_axis().name.data(), s.x_axis().name.size()) == "freq");
    CHECK(std::string(s.x_axis().unit.data(), s.x_axis().unit.size()) == "Hz");
}

TEST_CASE("span2d set_y_axis updates axis info", "[span2d]") {
    span2d<double> s;
    axis_info yi{ memepp::string{"amplitude"}, memepp::string{"dB"} };
    s.set_y_axis(yi);
    CHECK(std::string(s.y_axis().name.data(), s.y_axis().name.size()) == "amplitude");
    CHECK(std::string(s.y_axis().unit.data(), s.y_axis().unit.size()) == "dB");
}
