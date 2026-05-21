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

    struct PhysicalAddress {
        uint16_t channel = 0;
        uint16_t die = 0;
        uint32_t block = 0;
        uint32_t page = 0;

        [[nodiscard]] uint64_t to_linear_page() const noexcept {
            constexpr uint64_t pages_per_die = static_cast<uint64_t>(NandGeometry::BLOCKS_PER_DIE) * NandGeometry::PAGES_PER_BLOCK; // 131,072
            constexpr uint64_t pages_per_channel = pages_per_die * NandGeometry::DIES_PER_CHANNEL;

            return (channel * pages_per_channel) + (die * pages_per_die) + (static_cast<uint64_t>(block) * NandGeometry::PAGES_PER_BLOCK) + page;
        }

        [[nodiscard]] static PhysicalAddress from_linear_page(uint64_t linear) noexcept {
            constexpr uint64_t pages_per_die = static_cast<uint64_t>(NandGeometry::BLOCKS_PER_DIE) * NandGeometry::PAGES_PER_BLOCK; // 131,072
            constexpr uint64_t pages_per_channel = pages_per_die * NandGeometry::DIES_PER_CHANNEL;

            PhysicalAddress a;
            a.channel = static_cast<uint16_t>(linear / pages_per_channel);
            linear %= pages_per_channel;

            a.die = static_cast<uint16_t>(linear / pages_per_die);
            linear %= pages_per_die;

            a.block = static_cast<uint32_t>(linear / NandGeometry::PAGES_PER_BLOCK);
            a.page = static_cast<uint32_t>(linear % NandGeometry::PAGES_PER_BLOCK);

            return a;
        }

        [[nodiscard]] uint64_t to_linear_block() const noexcept {
            return (static_cast<uint64_t>(channel) * NandGeometry::DIES_PER_CHANNEL * NandGeometry::BLOCKS_PER_DIE)
                 + (static_cast<uint64_t>(die) * NandGeometry::BLOCKS_PER_DIE)
                 + block;
        }

        bool operator==(const PhysicalAddress& o) const noexcept {
            return channel == o.channel && die == o.die && block == o.block && page == o.page;
        }
    };

    inline constexpr PhysicalAddress PPA_INVALID{0xFFFF, 0xFFFF, 0xFFFFFFFF, 0xFFFFFFFF};

    enum class PageState: uint8_t {
        ERASED = 0xFF,
        VALID = 0x01,
        INVALID = 0x00
    };

    struct BlockMeta {
        uint32_t erase_count = 0;
        uint32_t valid_page_count = 0;
        uint32_t next_free_page = 0;
        PageState page_states[NandGeometry:: PAGES_PER_BLOCK];

        BlockMeta() {
            std::fill(std::begin(page_states), std::end(page_states), PageState::ERASED);
        }

        [[nodiscard]] bool is_full() const noexcept {
            return next_free_page >= NandGeometry::PAGES_PER_BLOCK;
        }

        [[nodiscard]] bool is_empty() const noexcept {
            return valid_page_count == 0 && next_free_page == 0;
        }

        [[nodiscard]] uint32_t garbage_count() const noexcept {
            return next_free_page - valid_page_count;
        }
    };

    enum class TempStream : uint8_t {
        HOT = 0,
        WARM = 1,
        COLD = 2,
        NUM_STREAMS = 3
    };

    inline const char* stream_name(TempStream s) noexcept {
        switch(s) {
            case TempStream::HOT: return "HOT";
            case TempStream::WARM: return "WARM";
            case TempStream::COLD: return "COLD";
            default: return "???";
        }
    }

    struct OracleConfig {
        float lambda = 0.001f; // Temporal decay rate
        float hot_threshold = 0.7f; // (Normalized score >= this) == HOT
        float warm_threshold = 0.3f; // (Normalized score >= this) == WARM, below COLD
        uint32_t trace_window = 1 << 20; // 1M entries in circular buffer
        uint32_t max_lba_count = 1 << 20; // Max unique LBAs tracked
    };

    struct TraceEntry {
        uint64_t lba;
        uint64_t timestamp; // Monotonic counter (e.g., I/O sequence #)
    };

    struct FTLStats {
        uint64_t host_write_pages = 0;
        uint64_t nand_write_pages = 0;
        uint64_t host_read_pages = 0;
        uint64_t nand_read_pages = 0;
        uint64_t gc_invocations = 0;
        uint64_t gc_pages_moved = 0;
        uint64_t total_erases = 0;

        [[nodiscard]] double write_amplification() const noexcept {
            return (host_write_pages == 0) ? 1.0 : static_cast<double>(nand_write_pages) / host_write_pages;
        }
    };
}