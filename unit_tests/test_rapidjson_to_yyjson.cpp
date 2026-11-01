#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <mmutilspp/openext/rapidjson/to_yyjson_val.h>
#include <yyjson.h>
#include <rapidjson/document.h>
#include <string>

// RAII wrapper for yyjson_mut_doc.
struct mut_doc_guard {
    yyjson_mut_doc* doc;
    mut_doc_guard() : doc(yyjson_mut_doc_new(nullptr)) {}
    ~mut_doc_guard() { if (doc) yyjson_mut_doc_free(doc); }
};

using mmupp::openext::to_yyjson_val;

// Parse JSON into a rapidjson::Document, convert, then write with yyjson and
// re-parse — round-trip comparison proves the conversion is lossless.
static std::string round_trip(const char* json)
{
    rapidjson::Document src;
    src.Parse(json);
    REQUIRE_FALSE(src.HasParseError());

    mut_doc_guard g;
    yyjson_mut_val* v = to_yyjson_val(g.doc, src);
    REQUIRE(v != nullptr);
    g.doc->root = v;  // yyjson_mut_write serializes doc->root

    char* out = yyjson_mut_write(g.doc, 0, nullptr);
    REQUIRE(out != nullptr);
    std::string result(out);
    free(out);
    return result;
}

// ---------------------------------------------------------------------------
// scalars
// ---------------------------------------------------------------------------

TEST_CASE("to_yyjson_val converts null", "[to_yyjson]") {
    CHECK(round_trip("null") == "null");
}

TEST_CASE("to_yyjson_val converts booleans", "[to_yyjson]") {
    CHECK(round_trip("true") == "true");
    CHECK(round_trip("false") == "false");
}

TEST_CASE("to_yyjson_val converts integers", "[to_yyjson]") {
    CHECK(round_trip("0") == "0");
    CHECK(round_trip("42") == "42");
    CHECK(round_trip("-42") == "-42");
}

TEST_CASE("to_yyjson_val converts int64 boundary values", "[to_yyjson]") {
    // 2^53+1 exceeds double precision; must survive as integer.
    CHECK(round_trip("9007199254740993") == "9007199254740993");
    CHECK(round_trip("-9007199254740993") == "-9007199254740993");
}

TEST_CASE("to_yyjson_val converts uint64 above int64 range", "[to_yyjson]") {
    // 2^63 fits uint64 only.
    CHECK(round_trip("9223372036854775808") == "9223372036854775808");
}

TEST_CASE("to_yyjson_val converts doubles", "[to_yyjson]") {
    // double round-trip: compare via re-parse, not string equality.
    auto out = round_trip("2.5");
    yyjson_doc* d = yyjson_read(out.c_str(), out.size(), 0);
    REQUIRE(d);
    CHECK(yyjson_get_real(yyjson_doc_get_root(d)) == Catch::Approx(2.5));
    yyjson_doc_free(d);
}

TEST_CASE("to_yyjson_val converts double that is integral-valued", "[to_yyjson]") {
    // rapidjson flags integral-valued doubles as IsDouble first in our order;
    // value must be preserved either way.
    auto out = round_trip("3.0");
    yyjson_doc* d = yyjson_read(out.c_str(), out.size(), 0);
    REQUIRE(d);
    yyjson_val* r = yyjson_doc_get_root(d);
    CHECK(yyjson_is_num(r));
    CHECK(yyjson_get_num(r) == Catch::Approx(3.0));
    yyjson_doc_free(d);
}

TEST_CASE("to_yyjson_val converts strings", "[to_yyjson]") {
    CHECK(round_trip("\"hello\"") == "\"hello\"");
    CHECK(round_trip("\"\"") == "\"\"");
}

TEST_CASE("to_yyjson_val preserves embedded NUL in strings", "[to_yyjson]") {
    // "a\0b" as JSON escape \u0000 — length must be kept via GetStringLength.
    rapidjson::Document src;
    src.Parse("\"a\\u0000b\"");
    REQUIRE_FALSE(src.HasParseError());
    REQUIRE(src.GetStringLength() == 3);

    mut_doc_guard g;
    yyjson_mut_val* v = to_yyjson_val(g.doc, src);
    REQUIRE(v != nullptr);
    REQUIRE(yyjson_mut_is_str(v));
    REQUIRE(unsafe_yyjson_get_len(v) == 3);

    // Compare raw bytes (string contains an embedded NUL).
    std::string raw(reinterpret_cast<const char*>(v->uni.ptr), 3);
    CHECK(raw == std::string("a\0b", 3));
}

TEST_CASE("to_yyjson_val converts non-ASCII strings", "[to_yyjson]") {
    auto out = round_trip("\"\\u4e2d\\u6587\"");  // Chinese
    yyjson_doc* d = yyjson_read(out.c_str(), out.size(), 0);
    REQUIRE(d);
    std::string s = yyjson_get_str(yyjson_doc_get_root(d));
    CHECK(s == "\xE4\xB8\xAD\xE6\x96\x87");
    yyjson_doc_free(d);
}

// ---------------------------------------------------------------------------
// containers
// ---------------------------------------------------------------------------

TEST_CASE("to_yyjson_val converts empty array and object", "[to_yyjson]") {
    CHECK(round_trip("[]") == "[]");
    CHECK(round_trip("{}") == "{}");
}

TEST_CASE("to_yyjson_val converts flat array", "[to_yyjson]") {
    CHECK(round_trip("[1,2,3]") == "[1,2,3]");
    CHECK(round_trip("[\"a\",\"b\"]") == "[\"a\",\"b\"]");
    CHECK(round_trip("[true,false,null]") == "[true,false,null]");
}

TEST_CASE("to_yyjson_val converts flat object", "[to_yyjson]") {
    auto out = round_trip("{\"x\":1,\"y\":\"s\"}");
    yyjson_doc* d = yyjson_read(out.c_str(), out.size(), 0);
    REQUIRE(d);
    yyjson_val* r = yyjson_doc_get_root(d);
    CHECK((yyjson_obj_get(r, "x") != nullptr && yyjson_get_int(yyjson_obj_get(r, "x")) == 1));
    CHECK((yyjson_obj_get(r, "y") != nullptr && std::string(yyjson_get_str(yyjson_obj_get(r, "y"))) == "s"));
    yyjson_doc_free(d);
}

TEST_CASE("to_yyjson_val converts nested containers", "[to_yyjson]") {
    const char* json = R"({"a":[1,{"b":[true,null]}],"c":{"d":2.5}})";
    auto out = round_trip(json);
    yyjson_doc* d = yyjson_read(out.c_str(), out.size(), 0);
    REQUIRE(d);
    yyjson_val* r = yyjson_doc_get_root(d);
    yyjson_val* a = yyjson_obj_get(r, "a");
    REQUIRE(yyjson_is_arr(a));
    CHECK(yyjson_arr_size(a) == 2);
    yyjson_val* inner = yyjson_arr_get(a, 1);
    REQUIRE(yyjson_is_obj(inner));
    yyjson_val* b = yyjson_obj_get(inner, "b");
    REQUIRE(b);
    CHECK(yyjson_arr_size(b) == 2);
    yyjson_doc_free(d);
}

TEST_CASE("to_yyjson_val round-trips a full document", "[to_yyjson]") {
    const char* json = R"({
        "name": "meme",
        "version": 2,
        "ratio": 0.75,
        "ok": true,
        "tags": ["x", "y"],
        "meta": { "id": 9007199254740993, "note": null }
    })";
    auto out = round_trip(json);
    yyjson_doc* d = yyjson_read(out.c_str(), out.size(), 0);
    REQUIRE(d);
    yyjson_val* r = yyjson_doc_get_root(d);
    CHECK(std::string(yyjson_get_str(yyjson_obj_get(r, "name"))) == "meme");
    CHECK(yyjson_get_int(yyjson_obj_get(r, "version")) == 2);
    CHECK(yyjson_get_real(yyjson_obj_get(r, "ratio")) == Catch::Approx(0.75));
    CHECK(yyjson_is_true(yyjson_obj_get(r, "ok")));
    yyjson_val* meta = yyjson_obj_get(r, "meta");
    REQUIRE(meta);
    // int64 precision preserved through conversion
    CHECK(yyjson_get_sint(yyjson_obj_get(meta, "id")) == 9007199254740993LL);
    CHECK(yyjson_is_null(yyjson_obj_get(meta, "note")));
    yyjson_doc_free(d);
}
