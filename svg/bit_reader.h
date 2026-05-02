#pragma once

#include "bit_hacks.h"

namespace waavs
{
    //
// getbitbyteoffset()
// 
// Given a bit number, calculate which byte
// it would be in, and which bit within that
// byte.
    constexpr void getbitbyteoffset(size_t bitnumber, size_t& byteoffset, size_t& bitoffset) noexcept
    {
        byteoffset = (int)(bitnumber / 8);
        bitoffset = bitnumber % 8;
    }


    constexpr uint64_t bitsValueFromBytes(const uint8_t* bytes, const size_t startbit, const size_t bitcount, bool bigendian = false) noexcept
    {
        // Sanity check
        if (nullptr == bytes)
            return 0;

        uint64_t value = 0;

        if (bigendian) {
            for (int i = (int)bitcount; i >= 0; i--) {
                size_t byteoffset = 0;
                size_t bitoffset = 0;
                getbitbyteoffset(startbit + i, byteoffset, bitoffset);
                bool bitval = bit_is_set_u64(bytes[byteoffset], bitoffset);

                if (bitval) {
                    value = bit_set_u64(value, i);
                }
            }
        }
        else {
            for (size_t i = 0; i < bitcount; i++) {
                size_t byteoffset = 0;
                size_t bitoffset = 0;
                getbitbyteoffset(startbit + i, byteoffset, bitoffset);
                bool bitval = bit_is_set_u64(bytes[byteoffset], bitoffset);

                if (bitval) {
                    value = bit_set_u64(value, i);
                }
            }
        }

        return value;
    }
}