#pragma once

#include <cstdint>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <algorithm>

namespace smartftl {
    struct NandGeometry {
        static constexpr uint32_t NUM_CHANNELS = 4;
        static constexpr uint32_t DIES_PER_CHANNEL = 2;
        static constexpr uint32_t BLOCKS_PER_DIE = 512;
        static constexpr uint32_t PAGES_PER_BLOCK = 256;
        static constexpr uint32_t PAGE_SIZE_BYTES = 16384; // 16KB

        static constexpr uint32_t TOTAL_DIES = NUM_CHANNELS * DIES_PER_CHANNEL; // 8
        static constexpr uint32_t TOTAL_BLOCKS = TOTAL_DIES * BLOCKS_PER_DIE; // 4096
        static constexpr uint32_t TOTAL_PAGES = TOTAL_BLOCKS * PAGES_PER_BLOCK; // 1,048,576
        static constexpr uint64_t TOTAL_CAPACITY = static_cast<uint64_t>(TOTAL_PAGES) * PAGE_SIZE_BYTES; // 17,179,869,184 bytes (16GB)

        static constexpr uint64_t BLOCK_SIZE_BYTES = static_cast<uint64_t>(PAGES_PER_BLOCK) * PAGE_SIZE_BYTES; // 4,194,304 bytes (4MB)
    };
};