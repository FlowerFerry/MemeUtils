#include <catch2/catch_test_macros.hpp>
#include <mmutilspp/log/level.h>
#include <string>

using mmupp::log::level;
using mmupp::log::level_to_str;

TEST_CASE("log level enum values are sequential from 0", "[log][level]") {
    CHECK(static_cast<uint8_t>(level::trace) == 0);
    CHECK(static_cast<uint8_t>(level::debug) == 1);
    CHECK(static_cast<uint8_t>(level::info)  == 2);
    CHECK(static_cast<uint8_t>(level::warn)  == 3);
    CHECK(static_cast<uint8_t>(level::error) == 4);
    CHECK(static_cast<uint8_t>(level::fatal) == 5);
    CHECK(static_cast<uint8_t>(level::off)   == 6);
}

TEST_CASE("level_to_str returns correct string for trace", "[log][level]") {
    CHECK(std::string(level_to_str(level::trace)) == "TRACE");
}

TEST_CASE("level_to_str returns correct string for debug", "[log][level]") {
    CHECK(std::string(level_to_str(level::debug)) == "DEBUG");
}

TEST_CASE("level_to_str returns correct string for info", "[log][level]") {
    CHECK(std::string(level_to_str(level::info)) == "INFO");
}

TEST_CASE("level_to_str returns correct string for warn", "[log][level]") {
    CHECK(std::string(level_to_str(level::warn)) == "WARN");
}

TEST_CASE("level_to_str returns correct string for error", "[log][level]") {
    CHECK(std::string(level_to_str(level::error)) == "ERROR");
}

TEST_CASE("level_to_str returns correct string for fatal", "[log][level]") {
    CHECK(std::string(level_to_str(level::fatal)) == "FATAL");
}

TEST_CASE("level_to_str returns correct string for off", "[log][level]") {
    CHECK(std::string(level_to_str(level::off)) == "OFF");
}

TEST_CASE("level_to_str returns UNKNOWN for out-of-range cast value", "[log][level]") {
    auto unknown = static_cast<level>(99);
    CHECK(std::string(level_to_str(unknown)) == "UNKNOWN");
}

TEST_CASE("level_to_str return value is a valid non-null C-string", "[log][level]") {
    CHECK(level_to_str(level::info) != nullptr);
    CHECK(level_to_str(static_cast<level>(200)) != nullptr);
}
