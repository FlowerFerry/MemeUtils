#include <catch2/catch_test_macros.hpp>
#include <mmutils/chksum/crc16_ccitt_false.h>
#include <cstdint>
#include <cstring>

TEST_CASE("CRC16 CCITT-FALSE standard test vector '123456789'", "[crc16]") {
    // Standard CCITT-FALSE: poly=0x1021, init=0xFFFF, expected=0x29B1
    const uint8_t data[] = {'1','2','3','4','5','6','7','8','9'};
    uint16_t crc = mmu_calc_crc16_ccitt_false(data, sizeof(data), 0xFFFF);
    CHECK(crc == 0x29B1);
}

TEST_CASE("CRC16 CCITT-FALSE zero-length input returns init value unchanged", "[crc16]") {
    const uint8_t placeholder = 0;
    CHECK(mmu_calc_crc16_ccitt_false(&placeholder, 0, 0xFFFF) == 0xFFFF);
    CHECK(mmu_calc_crc16_ccitt_false(&placeholder, 0, 0x0000) == 0x0000);
    CHECK(mmu_calc_crc16_ccitt_false(&placeholder, 0, 0x1234) == 0x1234);
}

TEST_CASE("CRC16 CCITT-FALSE different init values produce different results for same data", "[crc16]") {
    const uint8_t data[] = {0xAB, 0xCD};
    uint16_t crc_a = mmu_calc_crc16_ccitt_false(data, sizeof(data), 0xFFFF);
    uint16_t crc_b = mmu_calc_crc16_ccitt_false(data, sizeof(data), 0x0000);
    CHECK(crc_a != crc_b);
}

TEST_CASE("CRC16 CCITT-FALSE is deterministic for the same input", "[crc16]") {
    const uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    uint16_t first  = mmu_calc_crc16_ccitt_false(data, sizeof(data), 0xFFFF);
    uint16_t second = mmu_calc_crc16_ccitt_false(data, sizeof(data), 0xFFFF);
    CHECK(first == second);
}

TEST_CASE("CRC16 CCITT-FALSE single byte modifies CRC from init value", "[crc16]") {
    const uint8_t data[] = {0x42};
    uint16_t crc = mmu_calc_crc16_ccitt_false(data, 1, 0xFFFF);
    CHECK(crc != 0xFFFF);
}

TEST_CASE("CRC16 CCITT-FALSE different data produces different CRC", "[crc16]") {
    const uint8_t a[] = {0x01};
    const uint8_t b[] = {0x02};
    CHECK(mmu_calc_crc16_ccitt_false(a, 1, 0xFFFF) !=
          mmu_calc_crc16_ccitt_false(b, 1, 0xFFFF));
}

TEST_CASE("CRC16 CCITT-FALSE chained computation equals single computation", "[crc16]") {
    // Computing CRC incrementally should equal computing it all at once
    const uint8_t full[] = {0x11, 0x22, 0x33, 0x44};
    uint16_t crc_full = mmu_calc_crc16_ccitt_false(full, 4, 0xFFFF);

    // Chain: first 2 bytes, then last 2 bytes using previous result as init
    uint16_t crc_part1 = mmu_calc_crc16_ccitt_false(full,     2, 0xFFFF);
    uint16_t crc_chain = mmu_calc_crc16_ccitt_false(full + 2, 2, crc_part1);

    CHECK(crc_full == crc_chain);
}

TEST_CASE("CRC16 CCITT-FALSE_l and _b are byte-reverses of each other", "[crc16]") {
    const uint8_t data[] = {0xDE, 0xAD, 0xBE, 0xEF};
    uint16_t crc_l = mmu_calc_crc16_ccitt_false_l(data, sizeof(data));
    uint16_t crc_b = mmu_calc_crc16_ccitt_false_b(data, sizeof(data));
    uint16_t swapped = static_cast<uint16_t>((crc_l << 8) | (crc_l >> 8));
    CHECK(crc_b == swapped);
}

TEST_CASE("CRC16 CCITT-FALSE_l standard vector on little-endian host", "[crc16]") {
#if MEGO_ENDIAN__LITTLE_BYTE
    // On a little-endian machine, _l returns the CRC in its native (big-endian
    // value) form without byte-swapping.
    const uint8_t data[] = {'1','2','3','4','5','6','7','8','9'};
    uint16_t crc = mmu_calc_crc16_ccitt_false_l(data, sizeof(data));
    CHECK(crc == 0x29B1);
#else
    SUCCEED("Skipped: not a little-endian platform");
#endif
}

TEST_CASE("CRC16 CCITT-FALSE_b standard vector on little-endian host", "[crc16]") {
#if MEGO_ENDIAN__LITTLE_BYTE
    // On a little-endian machine, _b byte-swaps the result for big-endian wire order.
    const uint8_t data[] = {'1','2','3','4','5','6','7','8','9'};
    uint16_t crc = mmu_calc_crc16_ccitt_false_b(data, sizeof(data));
    uint16_t expected = static_cast<uint16_t>((0x29B1 << 8) | (0x29B1 >> 8));
    CHECK(crc == expected);
#else
    SUCCEED("Skipped: not a little-endian platform");
#endif
}
