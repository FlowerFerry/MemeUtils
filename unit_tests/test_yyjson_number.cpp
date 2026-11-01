#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <mmutilspp/openext/yyjson/number.h>
#include <yyjson.h>
#include <cstring>
#include <string>

// RAII wrapper for yyjson_doc.
struct doc_guard {
    yyjson_doc* doc;
    explicit doc_guard(const char* json) : doc(yyjson_read(json, std::strlen(json), 0)) {}
    ~doc_guard() { if (doc) yyjson_doc_free(doc); }
    yyjson_val* root() const { return yyjson_doc_get_root(doc); }
};

using mmupp::openext::get_num_with_default;

// ---------------------------------------------------------------------------
// missing key -> default
// ---------------------------------------------------------------------------

TEST_CASE("get_num_with_default returns default when key absent", "[yyjson_number]") {
    doc_guard d{ R"({"a":1})" };
    REQUIRE(d.root());
    CHECK(get_num_with_default(d.root(), "missing", -1, 42) == 42);
}

TEST_CASE("get_num_with_default returns default for null key with negative slen", "[yyjson_number]") {
    doc_guard d{ R"({"a":1})" };
    REQUIRE(d.root());
    CHECK(get_num_with_default(d.root(), nullptr, -1, 7) == 7);
}

// ---------------------------------------------------------------------------
// number types
// ---------------------------------------------------------------------------

TEST_CASE("get_num_with_default reads integer", "[yyjson_number]") {
    doc_guard d{ R"({"n":123})" };
    CHECK(get_num_with_default(d.root(), "n", -1, 0) == 123);
}

TEST_CASE("get_num_with_default reads negative integer", "[yyjson_number]") {
    doc_guard d{ R"({"n":-45})" };
    CHECK(get_num_with_default(d.root(), "n", -1, 0) == -45);
}

TEST_CASE("get_num_with_default reads double into int (truncates)", "[yyjson_number]") {
    doc_guard d{ R"({"n":3.9})" };
    CHECK(get_num_with_default(d.root(), "n", -1, 0) == 3);
}

TEST_CASE("get_num_with_default reads double into double", "[yyjson_number]") {
    doc_guard d{ R"({"n":2.5})" };
    CHECK(get_num_with_default(d.root(), "n", -1, 0.0) == Catch::Approx(2.5));
}

TEST_CASE("get_num_with_default narrows double to uint8_t", "[yyjson_number]") {
    doc_guard d{ R"({"n":300.0})" };
    CHECK(get_num_with_default<uint8_t>(d.root(), "n", -1, 0) == static_cast<uint8_t>(300));
}

// ---------------------------------------------------------------------------
// bool -> 0/1
// ---------------------------------------------------------------------------

TEST_CASE("get_num_with_default maps true to 1", "[yyjson_number]") {
    doc_guard d{ R"({"b":true})" };
    CHECK(get_num_with_default(d.root(), "b", -1, 9) == 1);
}

TEST_CASE("get_num_with_default maps false to 0", "[yyjson_number]") {
    doc_guard d{ R"({"b":false})" };
    CHECK(get_num_with_default(d.root(), "b", -1, 9) == 0);
}

// ---------------------------------------------------------------------------
// string -> stod
// ---------------------------------------------------------------------------

TEST_CASE("get_num_with_default parses numeric string", "[yyjson_number]") {
    doc_guard d{ R"({"s":"6.5"})" };
    CHECK(get_num_with_default(d.root(), "s", -1, 0.0) == Catch::Approx(6.5));
}

TEST_CASE("get_num_with_default parses integer string into int", "[yyjson_number]") {
    doc_guard d{ R"({"s":"77"})" };
    CHECK(get_num_with_default(d.root(), "s", -1, 0) == 77);
}

TEST_CASE("get_num_with_default throws on non-numeric string (stod)", "[yyjson_number]") {
    doc_guard d{ R"({"s":"abc"})" };
    CHECK_THROWS_AS(get_num_with_default(d.root(), "s", -1, 0), std::invalid_argument);
}

// ---------------------------------------------------------------------------
// other JSON types -> default
// ---------------------------------------------------------------------------

TEST_CASE("get_num_with_default returns default for null value", "[yyjson_number]") {
    doc_guard d{ R"({"v":null})" };
    CHECK(get_num_with_default(d.root(), "v", -1, 5) == 5);
}

TEST_CASE("get_num_with_default returns default for array value", "[yyjson_number]") {
    doc_guard d{ R"({"v":[1,2]})" };
    CHECK(get_num_with_default(d.root(), "v", -1, 5) == 5);
}

TEST_CASE("get_num_with_default returns default for object value", "[yyjson_number]") {
    doc_guard d{ R"({"v":{"x":1}})" };
    CHECK(get_num_with_default(d.root(), "v", -1, 5) == 5);
}

// ---------------------------------------------------------------------------
// explicit key length (slen >= 0)
// ---------------------------------------------------------------------------

TEST_CASE("get_num_with_default uses explicit key length", "[yyjson_number]") {
    doc_guard d{ R"({"abc":9})" };
    // key length 3: "abc" matches
    CHECK(get_num_with_default(d.root(), "abc", 3, 0) == 9);
    // key length 2: "ab" does not match "abc"
    CHECK(get_num_with_default(d.root(), "abc", 2, 0) == 0);
}

TEST_CASE("get_num_with_default explicit length beyond NUL matches", "[yyjson_number]") {
    doc_guard d{ R"({"abc":9})" };
    // NUL inside the key buffer, length covers only "abc"
    const char key[] = { 'a', 'b', 'c', '\0', 'X' };
    CHECK(get_num_with_default(d.root(), key, 3, 0) == 9);
}
