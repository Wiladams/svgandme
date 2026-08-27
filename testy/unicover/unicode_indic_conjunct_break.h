// unicode_indic_conjunct_break.h
#pragma once

#include <cstdint>
#include <type_traits>


namespace waavs
{
    enum class UnicodeIndicConjunctBreak : uint8_t
    {
        None = 0,
        Consonant = 1,
        Extend = 2,
        Linker = 3
    };


    static constexpr uint8_t kUnicodeIndicConjunctBreakCount = 4;


    [[nodiscard]]
    static constexpr bool unicodeIndicConjunctBreakIsValid(uint8_t value) noexcept {
        return value < kUnicodeIndicConjunctBreakCount;
    }


    [[nodiscard]]
    static constexpr bool unicodeIndicConjunctBreakIsValid(
        UnicodeIndicConjunctBreak value) noexcept
    {
        return unicodeIndicConjunctBreakIsValid(
            static_cast<uint8_t>(value));
    }


    static_assert(sizeof(UnicodeIndicConjunctBreak) == 1,
        "UnicodeIndicConjunctBreak must be exactly one byte");

    static_assert(
        std::is_same<
        std::underlying_type<UnicodeIndicConjunctBreak>::type,
        uint8_t>::value,
        "UnicodeIndicConjunctBreak must use uint8_t storage");

    static_assert(
        static_cast<uint8_t>(UnicodeIndicConjunctBreak::None) == 0,
        "Unicode Indic_Conjunct_Break None must remain value zero");

    static_assert(
        static_cast<uint8_t>(UnicodeIndicConjunctBreak::Linker) + 1u ==
        kUnicodeIndicConjunctBreakCount,
        "Unicode Indic_Conjunct_Break count is inconsistent");

} // namespace waavs