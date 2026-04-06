#include <catch2/catch_test_macros.hpp>
#include <mmutilspp/fs/program_path.hpp>
#include <mego/predef/os/windows.h>

using mmupp::fs::program_file_path;
using mmupp::fs::program_directory_path;
using mmupp::fs::relative_with_program_path;

TEST_CASE("program_file_path returns non-empty string", "[program_path]") {
    auto p = program_file_path();
    CHECK_FALSE(p.empty());
}

TEST_CASE("program_directory_path returns non-empty string", "[program_path]") {
    auto p = program_directory_path();
    CHECK_FALSE(p.empty());
}

TEST_CASE("program_file_path does not end with a path separator", "[program_path]") {
    auto p = program_file_path();
    REQUIRE_FALSE(p.empty());
    char last = p.data()[p.size() - 1];
    CHECK(last != '/');
    CHECK(last != '\\');
}

TEST_CASE("program_directory_path does not end with a path separator", "[program_path]") {
    auto p = program_directory_path();
    REQUIRE_FALSE(p.empty());
    char last = p.data()[p.size() - 1];
    CHECK(last != '/');
    CHECK(last != '\\');
}

TEST_CASE("program_file_path contains program_directory_path as a prefix", "[program_path]") {
    auto file = program_file_path();
    auto dir  = program_directory_path();
    REQUIRE_FALSE(file.empty());
    REQUIRE_FALSE(dir.empty());
    // file path must be strictly longer than directory path
    CHECK(file.size() > dir.size());
    // the directory portion of the file path must match
    memepp::string_view file_sv{ file.data(), file.size() };
    memepp::string_view dir_sv { dir.data(),  dir.size()  };
    CHECK(file_sv.starts_with(dir_sv));
}

TEST_CASE("relative_with_program_path: empty string returns empty", "[program_path]") {
    auto r = relative_with_program_path(memepp::string_view{});
    CHECK(r.empty());
}

TEST_CASE("relative_with_program_path: whitespace-only returns empty", "[program_path]") {
    memepp::string ws{ "   " };
    auto r = relative_with_program_path(memepp::string_view{ ws.data(), ws.size() });
    CHECK(r.empty());
}

TEST_CASE("relative_with_program_path: absolute unix path returned as-is", "[program_path]") {
    memepp::string input{ "/usr/local/bin" };
    auto r = relative_with_program_path(memepp::string_view{ input.data(), input.size() });
    CHECK(std::string(r.data(), r.size()) == "/usr/local/bin");
}

#if MG_OS__WIN_AVAIL

TEST_CASE("relative_with_program_path: backslash absolute path returned as-is", "[program_path]") {
    memepp::string input{ "\\Windows\\System32" };
    auto r = relative_with_program_path(memepp::string_view{ input.data(), input.size() });
    CHECK(std::string(r.data(), r.size()) == "\\Windows\\System32");
}

TEST_CASE("relative_with_program_path: drive letter with forward slash returned as-is", "[program_path]") {
    memepp::string input{ "C:/Windows" };
    auto r = relative_with_program_path(memepp::string_view{ input.data(), input.size() });
    CHECK(std::string(r.data(), r.size()) == "C:/Windows");
}

TEST_CASE("relative_with_program_path: drive letter with backslash returned as-is", "[program_path]") {
    memepp::string input{ "C:\\Windows" };
    auto r = relative_with_program_path(memepp::string_view{ input.data(), input.size() });
    CHECK(std::string(r.data(), r.size()) == "C:\\Windows");
}

TEST_CASE("relative_with_program_path: LRO prefix is stripped from drive-letter path", "[program_path]") {
    // U+202A LEFT-TO-RIGHT EMBEDDING encoded as UTF-8: 0xE2 0x80 0xAA
    // Split string literal to prevent \x from greedily consuming 'C' as a hex digit.
    std::string lro_prefix_str = "\xE2\x80\xAA" "C:/dir";
    memepp::string input{ lro_prefix_str.c_str(), static_cast<int>(lro_prefix_str.size()) };
    auto r = relative_with_program_path(memepp::string_view{ input.data(), input.size() });
    CHECK(std::string(r.data(), r.size()) == "C:/dir");
}

#endif // MG_OS__WIN_AVAIL

TEST_CASE("relative_with_program_path: relative path is joined to program directory", "[program_path]") {
    memepp::string rel{ "sub/file.txt" };
    auto r = relative_with_program_path(memepp::string_view{ rel.data(), rel.size() });
    auto dir = program_directory_path();

    REQUIRE_FALSE(r.empty());

    memepp::string_view r_sv  { r.data(),   r.size()   };
    memepp::string_view dir_sv{ dir.data(), dir.size() };
    CHECK(r_sv.starts_with(dir_sv));
    CHECK(r_sv.ends_with(memepp::string_view{ rel.data(), rel.size() }));
}
