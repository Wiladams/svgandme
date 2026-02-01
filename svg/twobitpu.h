#pragma once



#include <cstdint>
#include <type_traits>

namespace waavs
{
    /*
        Two Bit Processing Unit (TBPU)
        This is a core implementation of a processing unit that has
        two bit opcodes.  The opcode '0' is meant as a program terminator,
        so, there are actually only 3 valid opcodes (1, 2, 3).
    */

    template <typename StorageT>
    struct TwoBitProcessingUnit
    {
        static_assert(std::is_integral<StorageT>::value, "StorageT must be an integral type");
        static_assert(std::is_unsigned<StorageT>::value, "StorageT must be unsigned (bit packing)");

    public:
        using Storage = StorageT;

        static constexpr uint32_t kBitsPerOp = 2u;
        static constexpr uint32_t kMaxOps = uint32_t(sizeof(Storage) * 8 / kBitsPerOp);
        static constexpr uint32_t kOpcodeMask = 0x03u;   // mask for a single opcode (2 bits)

        // 00 is reserved as END / NOOP
        enum : uint32_t {
            OP_END = 0u,     // 00
            OP_1 = 1u,       // 01
            OP_2 = 2u,       // 10
            OP_3 = 3u        // 11
        };

        constexpr TwoBitProcessingUnit() noexcept = default;
        constexpr explicit TwoBitProcessingUnit(Storage code) noexcept : fCode(code) {}

        constexpr Storage code() const noexcept { return fCode; }
        constexpr void setCode(Storage code) noexcept { fCode = code; }

        // Write a particular value to a given slot
        constexpr bool write(uint32_t op, uint32_t slot) noexcept
        {
            if (slot >= kMaxOps) return false;

            const uint32_t shift = slot * kBitsPerOp;
            const Storage mask = Storage(kOpcodeMask) << shift;

            // clear existing 2 bits in that slot and...
            fCode = Storage(fCode & ~mask);

            // ...set new opcode bits
            fCode = Storage(fCode | (Storage(op & kOpcodeMask) << shift));
            return true;
        }


        // Append an opcode at a given slot
        constexpr bool emit(uint32_t op, uint32_t slot) noexcept
        {
            return write(op, slot);
        }

        // Explicit terminator (00).  Usuallly a no-op, but documents intent
        constexpr bool terminate(uint32_t slot) noexcept
        {
            return write(OP_END, slot);
        }

        // Generic execution loop
        template <class Executor>
        void run(Executor& exec) const noexcept
        {
            Storage p = fCode;

            for (uint32_t slot = 0; slot < kMaxOps; ++slot)
            {
                const uint32_t opCode = uint32_t(p & Storage(kOpcodeMask));   // get the next (lowest two bits) opcode
                if (opCode == OP_END)
                    break;

                exec.execute(opCode);

                p = Storage(p >> kBitsPerOp);
            }
        }
    private:
        Storage fCode{ 0 };
    };
}