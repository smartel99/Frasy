#ifndef FRASY_UTILS_MISC_CRC32_H
#define FRASY_UTILS_MISC_CRC32_H
#include <cstdint>
#include <vector>

void     crc32_clear();
uint32_t crc32_calculate(const uint8_t* data, std::size_t len);
uint32_t crc32_calculate(const std::vector<uint8_t>& data);
uint32_t crc32_calculate(const std::vector<std::vector<uint8_t>>& data);
uint32_t crc32_accumulate(const uint8_t* data, std::size_t len);
uint32_t crc32_accumulate(const std::vector<uint8_t>& data);
uint32_t crc32_finalize();

#endif    // FRASY_UTILS_MISC_CRC32_H
