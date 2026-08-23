#pragma once

#include "core_openhashmap.h"
#include "core_utf8.h"
#include "font_interfaces.h"
#include "opentype_types.h"
#include "opentype_bytestream.h"


namespace waavs {
    namespace opentype {
        class OpenTypeFaceData final : public IProvideFontFaceData, public IProvideOpenTypeTables
        {
        private:
            SharedMemBuff fSource;
            FontName fSourceLocation{ nullptr };

            size_t fFaceOffset{ 0 };

            WSOpenHashMap<Tag, TableRecord, WSHash32> fTables;
            WSOpenHashMap<uint64_t, const char*, WSHash64> fNameStrings;
            WSOpenHashMap<uint32_t, CmapEncoding, WSHash32> fCmapEncodings;

            UnicodeCoverage fUnicodeCoverage{};
            uint32_t fGlyphCount{ 0 };
            uint16_t fUnitsPerEm{ 0 };

            FontFaceProperties fProperties{};
            bool fValid{ false };

        public:
            OpenTypeFaceData(
                const SharedMemBuff& source,
                size_t faceOffset,
                FontName srcLocation = nullptr) noexcept
                : fSource(source)
                , fFaceOffset(faceOffset)
                , fSourceLocation(srcLocation)
            {
                parse();
            }

            bool isValid() const noexcept
            {
                return fValid;
            }

            // IProvideFontFaceData
            FontName sourceLocation() const noexcept override
            {
                return fSourceLocation;
            }

            FontName familyName() const noexcept override
            {
                return getNameString(NameIDs::NID_FAMILY);
            }

            FontName subfamilyName() const noexcept override
            {
                return getNameString(NameIDs::NID_SUBFAMILY);
            }

            FontName fullName() const noexcept override
            {
                return getNameString(NameIDs::NID_FULL_NAME);
            }

            FontName postScriptName() const noexcept override
            {
                return getNameString(NameIDs::NID_POSTSCRIPT);
            }

            FontFaceProperties properties() const noexcept override
            {
                return fProperties;
            }

            uint32_t glyphCount() const noexcept override
            {
                return fGlyphCount;
            }

            uint16_t unitsPerEm() const noexcept override
            {
                return fUnitsPerEm;
            }

            uint32_t glyphIndex(uint32_t codepoint) const noexcept override
            {
                // use existing cmap lookup.
                //...
                return 0;
            }

            const UnicodeCoverage& unicodeCoverage() const noexcept override
            {
                return fUnicodeCoverage;
            }

            // OpenType-specific access

            const TableRecord* getTable(Tag tag) const noexcept override
            {
                return fTables.getRef(tag);
            }

            bool hasTable(Tag tag) const noexcept override
            {
                return fTables.contains(tag);
            }

        private:

            inline bool parse() noexcept
            {
                ByteSpan data(fSource.data(), fSource.size());
                OpenTypeByteStream stream(data);

                // Do NOT slice data at fFaceOffset.
                // TTC table offsets remain relative to the complete source.

                if (!stream.seek(fFaceOffset))
                    return false;

                uint16_t numTables = 0;

                if (!parseOffsetTable(stream, numTables))
                    return false;

                if (!parseTableDirectory(stream, numTables))
                    return false;

                if (!parseCoreTables())
                    return false;

                fValid = true;
                return true;
            }

            inline bool parseOffsetTable(OpenTypeByteStream& stream, uint16_t& numTables) noexcept
            {
                uint32_t version;

                if (!stream.readUInt32(version))
                    return false;
                /*
                if (version == 0x00010000 || version == 0x00020000)
                {
                    mIsTrueType = true;
                }
                else if (version == OTAG("OTTO"))
                {
                    mIsTrueType = false;
                }
                else if (version == OTAG("true"))
                {
                    mIsTrueType = true;
                }
                else if (version == OTAG("typ1"))
                {
                    mIsTrueType = false;
                }
                else
                {
                    return false;
                }
                */

                if (!stream.readUInt16(numTables))
                    return false;

                if (numTables == 0)
                    return false;

                // searchRange, entrySelector, rangeShift
                if (!stream.skip(6))
                    return false;

                // Each table-directory entry is 16 bytes.
                if (numTables > stream.remaining() / 16)
                    return false;

                return true;
            }

            inline bool parseTableDirectory(OpenTypeByteStream& stream, uint16_t numTables) noexcept
            {
                fTables.reserve(numTables);

                for (uint16_t i = 0; i < numTables; ++i)
                {
                    Tag tag;
                    uint32_t checksum;
                    Offset offset;
                    uint32_t length;

                    if (!stream.readUInt32(tag))
                        return false;

                    if (!stream.readUInt32(checksum))
                        return false;

                    if (!stream.readOffset32(offset))
                        return false;

                    if (!stream.readUInt32(length))
                        return false;

                    // THIS is the critical TTC behavior:
                    //
                    // mStream's base is still the beginning of the entire file.
                    // Therefore the table offset is resolved against the correct
                    // file-level coordinate system.
                    auto tableData = stream.subStream(offset, length);

                    if (!tableData.isValid())
                        return false;

                    TableRecord record{};
                    record.tag = tag;
                    record.checksum = checksum;
                    record.offset = offset;
                    record.length = length;
                    record.data = tableData.remainingData();

                    fTables.put(tag, std::move(record));
                }

                return true;
            }

            inline bool parseCoreTables() noexcept {
                const TableRecord* maxpTable = getTable(TagConstants::MAXP);
                if (maxpTable) parseMaxpTable(*maxpTable);

                const TableRecord* nameTable = getTable(TagConstants::NAME);
                if (nameTable) parseNameTable(*nameTable);

                const TableRecord* cmapTable = getTable(TagConstants::CMAP);
                if (cmapTable) parseCmapTable(*cmapTable);

                const TableRecord* headTable = getTable(TagConstants::HEAD);
                if (headTable) parseHeadTable(*headTable);

                return true;
            }

            // ==============================================
            // parseMaxpTable
            // ==============================================

            inline void parseMaxpTable(const TableRecord& table) noexcept
            {
                const ByteSpan& data = table.data;
                if (data.size() < 6)
                    return;

                OpenTypeByteStream stream(data);
                stream.skip(4); // Skip version
                uint16_t numGlyphs;
                if (stream.readUInt16(numGlyphs))
                    fGlyphCount = numGlyphs;
            }

            // ==============================================
            // parseHeadTable
            // ==============================================
            inline void parseHeadTable(const TableRecord& table) noexcept
            {
                const ByteSpan& data = table.data;
                if (data.size() < 12)
                    return;

                OpenTypeByteStream stream(data);
                stream.skip(8); // Skip version and revision

                uint32_t magic;
                if (!stream.readUInt32(magic)) return;

                if (magic != 0x5F0F3CF5) {
                    fValid = false;
                }
            }

            // ==============================================
            // parseCmapTable - not an official table, but substantial
            // enough to warrant its own parsing function.
            // =============================================
            inline void parseCmapCoverage(
                const OpenTypeByteStream& subtable,
                uint16_t format,
                UnicodeCoverageBuilder& coverage) noexcept
            {
                switch (format)
                {
                    // ============================================================
                    // Format 0
                    //
                    // uint16 format
                    // uint16 length
                    // uint16 language
                    // uint8  glyphIdArray[256]
                    // ============================================================
                case 0:
                {
                    auto glyphs = subtable.subStream(6);

                    if (!glyphs.isValid())
                        return;

                    for (uint32_t cp = 0; cp < 256; ++cp)
                    {
                        uint8_t glyphId = 0;

                        if (!glyphs.readUInt8(glyphId))
                            return;

                        if (glyphId != 0)
                            coverage.add(cp);
                    }

                    break;
                }

                // ============================================================
                // Format 4
                //
                // uint16 format
                // uint16 length
                // uint16 language
                // uint16 segCountX2
                // uint16 searchRange
                // uint16 entrySelector
                // uint16 rangeShift
                //
                // uint16 endCode[segCount]
                // uint16 reservedPad
                // uint16 startCode[segCount]
                // int16  idDelta[segCount]
                // uint16 idRangeOffset[segCount]
                // uint16 glyphIdArray[]
                // ============================================================
                case 4:
                {
                    auto header = subtable.subStream(6);

                    if (!header.isValid())
                        return;

                    uint16_t segCountX2 = 0;

                    if (!header.readUInt16(segCountX2))
                        return;

                    if ((segCountX2 & 1u) != 0)
                        return;

                    const uint32_t segCount =
                        uint32_t(segCountX2) >> 1;

                    if (segCount == 0)
                        return;

                    // Arrays begin after the 14-byte fixed header.
                    const size_t endCodeOffset = 14;

                    const size_t reservedPadOffset =
                        endCodeOffset +
                        size_t(segCount) * 2;

                    const size_t startCodeOffset =
                        reservedPadOffset + 2;

                    const size_t idDeltaOffset =
                        startCodeOffset +
                        size_t(segCount) * 2;

                    const size_t idRangeOffsetOffset =
                        idDeltaOffset +
                        size_t(segCount) * 2;

                    const size_t glyphArrayOffset =
                        idRangeOffsetOffset +
                        size_t(segCount) * 2;

                    // This validates that all four arrays fit.
                    auto glyphArray =
                        subtable.subStream(glyphArrayOffset);

                    if (!glyphArray.isValid())
                        return;

                    auto endCodes =
                        subtable.subStream(endCodeOffset);

                    auto startCodes =
                        subtable.subStream(startCodeOffset);

                    auto deltas =
                        subtable.subStream(idDeltaOffset);

                    auto rangeOffsets =
                        subtable.subStream(idRangeOffsetOffset);

                    if (!endCodes.isValid() ||
                        !startCodes.isValid() ||
                        !deltas.isValid() ||
                        !rangeOffsets.isValid())
                        return;

                    for (uint32_t i = 0; i < segCount; ++i)
                    {
                        uint16_t endCode = 0;
                        uint16_t startCode = 0;
                        uint16_t idDelta = 0;
                        uint16_t idRangeOffset = 0;

                        if (!endCodes.readUInt16(endCode))
                            return;

                        if (!startCodes.readUInt16(startCode))
                            return;

                        if (!deltas.readUInt16(idDelta))
                            return;

                        if (!rangeOffsets.readUInt16(idRangeOffset))
                            return;

                        if (startCode > endCode)
                            continue;

                        // Standard terminal segment.
                        if (startCode == 0xFFFFu &&
                            endCode == 0xFFFFu)
                            continue;

                        // ----------------------------------------------------
                        // idRangeOffset == 0
                        //
                        // glyph =
                        //   (codepoint + idDelta) mod 65536
                        //
                        // This means the entire segment is covered except
                        // possibly one codepoint whose result becomes glyph 0.
                        // ----------------------------------------------------
                        if (idRangeOffset == 0)
                        {
                            const uint16_t zeroCodePoint =
                                static_cast<uint16_t>(0u - idDelta);

                            if (zeroCodePoint < startCode ||
                                zeroCodePoint > endCode)
                            {
                                coverage.addRange(
                                    startCode,
                                    endCode);
                            }
                            else
                            {
                                if (startCode < zeroCodePoint)
                                {
                                    coverage.addRange(
                                        startCode,
                                        uint32_t(zeroCodePoint) - 1u);
                                }

                                if (zeroCodePoint < endCode)
                                {
                                    coverage.addRange(
                                        uint32_t(zeroCodePoint) + 1u,
                                        endCode);
                                }
                            }

                            continue;
                        }

                        // ----------------------------------------------------
                        // idRangeOffset != 0
                        //
                        // The offset is relative to the address of the
                        // particular idRangeOffset[i] word itself.
                        // ----------------------------------------------------

                        const size_t rangeOffsetWord =
                            idRangeOffsetOffset +
                            size_t(i) * 2;

                        for (uint32_t cp = startCode;
                            cp <= endCode;
                            ++cp)
                        {
                            const size_t glyphOffset =
                                rangeOffsetWord +
                                size_t(idRangeOffset) +
                                size_t(cp - startCode) * 2;

                            auto glyphStream =
                                subtable.subStream(glyphOffset);

                            if (!glyphStream.isValid())
                                break;

                            uint16_t glyphId = 0;

                            if (!glyphStream.readUInt16(glyphId))
                                break;

                            // A zero glyphIdArray entry remains zero.
                            // idDelta is only applied to a nonzero entry.
                            if (glyphId != 0)
                            {
                                glyphId =
                                    static_cast<uint16_t>(
                                        uint32_t(glyphId) +
                                        uint32_t(idDelta));

                                if (glyphId != 0)
                                    coverage.add(cp);
                            }

                            // Avoid wrap when endCode == 0xFFFF.
                            if (cp == 0xFFFFu)
                                break;
                        }
                    }

                    break;
                }

                // ============================================================
                // Format 6
                //
                // uint16 format
                // uint16 length
                // uint16 language
                // uint16 firstCode
                // uint16 entryCount
                // uint16 glyphIdArray[entryCount]
                // ============================================================
                case 6:
                {
                    auto stream = subtable.subStream(6);

                    if (!stream.isValid())
                        return;

                    uint16_t firstCode = 0;
                    uint16_t entryCount = 0;

                    if (!stream.readUInt16(firstCode))
                        return;

                    if (!stream.readUInt16(entryCount))
                        return;

                    if (entryCount > stream.remaining() / 2)
                        return;

                    for (uint32_t i = 0; i < entryCount; ++i)
                    {
                        uint16_t glyphId = 0;

                        if (!stream.readUInt16(glyphId))
                            return;

                        if (glyphId != 0)
                        {
                            coverage.add(
                                uint32_t(firstCode) + i);
                        }
                    }

                    break;
                }

                // ============================================================
                // Format 10
                //
                // uint16 format
                // uint16 reserved
                // uint32 length
                // uint32 language
                // uint32 startCharCode
                // uint32 numChars
                // uint16 glyphIdArray[numChars]
                // ============================================================
                case 10:
                {
                    auto stream = subtable.subStream(12);

                    if (!stream.isValid())
                        return;

                    uint32_t startCharCode = 0;
                    uint32_t numChars = 0;

                    if (!stream.readUInt32(startCharCode))
                        return;

                    if (!stream.readUInt32(numChars))
                        return;

                    if (numChars > stream.remaining() / 2)
                        return;

                    for (uint32_t i = 0; i < numChars; ++i)
                    {
                        uint16_t glyphId = 0;

                        if (!stream.readUInt16(glyphId))
                            return;

                        const uint64_t cp =
                            uint64_t(startCharCode) +
                            uint64_t(i);

                        if (cp >= UnicodeCoverage::kUnicodeLimit)
                            continue;

                        if (glyphId != 0)
                        {
                            coverage.add(
                                static_cast<uint32_t>(cp));
                        }
                    }

                    break;
                }

                // ============================================================
                // Format 12
                //
                // uint16 format
                // uint16 reserved
                // uint32 length
                // uint32 language
                // uint32 numGroups
                //
                // groups:
                //   uint32 startCharCode
                //   uint32 endCharCode
                //   uint32 startGlyphID
                // ============================================================
                case 12:
                {
                    auto stream = subtable.subStream(12);

                    if (!stream.isValid())
                        return;

                    uint32_t numGroups = 0;

                    if (!stream.readUInt32(numGroups))
                        return;

                    if (numGroups > stream.remaining() / 12)
                        return;

                    for (uint32_t i = 0; i < numGroups; ++i)
                    {
                        uint32_t startCharCode = 0;
                        uint32_t endCharCode = 0;
                        uint32_t startGlyphId = 0;

                        if (!stream.readUInt32(startCharCode))
                            return;

                        if (!stream.readUInt32(endCharCode))
                            return;

                        if (!stream.readUInt32(startGlyphId))
                            return;

                        if (startCharCode > endCharCode)
                            continue;

                        if (startCharCode >=
                            UnicodeCoverage::kUnicodeLimit)
                            continue;

                        if (endCharCode >=
                            UnicodeCoverage::kUnicodeLimit)
                        {
                            endCharCode =
                                UnicodeCoverage::kUnicodeLimit - 1u;
                        }

                        // Format 12 increments the glyph ID for each codepoint.
                        //
                        // If startGlyphId == 0, only the first codepoint maps
                        // to .notdef; the remainder are valid mappings.
                        if (startGlyphId == 0)
                        {
                            if (startCharCode < endCharCode)
                            {
                                coverage.addRange(
                                    startCharCode + 1u,
                                    endCharCode);
                            }
                        }
                        else
                        {
                            coverage.addRange(
                                startCharCode,
                                endCharCode);
                        }
                    }

                    break;
                }

                // ============================================================
                // Format 13
                //
                // uint16 format
                // uint16 reserved
                // uint32 length
                // uint32 language
                // uint32 numGroups
                //
                // groups:
                //   uint32 startCharCode
                //   uint32 endCharCode
                //   uint32 glyphID
                // ============================================================
                case 13:
                {
                    auto stream = subtable.subStream(12);

                    if (!stream.isValid())
                        return;

                    uint32_t numGroups = 0;

                    if (!stream.readUInt32(numGroups))
                        return;

                    if (numGroups > stream.remaining() / 12)
                        return;

                    for (uint32_t i = 0; i < numGroups; ++i)
                    {
                        uint32_t startCharCode = 0;
                        uint32_t endCharCode = 0;
                        uint32_t glyphId = 0;

                        if (!stream.readUInt32(startCharCode))
                            return;

                        if (!stream.readUInt32(endCharCode))
                            return;

                        if (!stream.readUInt32(glyphId))
                            return;

                        if (glyphId == 0)
                            continue;

                        if (startCharCode > endCharCode)
                            continue;

                        if (startCharCode >=
                            UnicodeCoverage::kUnicodeLimit)
                            continue;

                        if (endCharCode >=
                            UnicodeCoverage::kUnicodeLimit)
                        {
                            endCharCode =
                                UnicodeCoverage::kUnicodeLimit - 1u;
                        }

                        coverage.addRange(
                            startCharCode,
                            endCharCode);
                    }

                    break;
                }

                // Format 14 represents variation-selector sequences rather than
                // standalone Unicode scalar coverage.
                //
                // Formats 2 and 8 can be added later if they prove useful for
                // fonts encountered in the wild.
                case 2:
                case 8:
                case 14:
                default:
                    break;
                }
            }

            // ==============================================
            // parseCmapTable
            // ==============================================

            inline void parseCmapTable(const TableRecord& table) noexcept
            {
                const ByteSpan& data = table.data;

                if (data.size() < 4)
                    return;

                OpenTypeByteStream stream(data);

                uint16_t version = 0;
                uint16_t numEncodings = 0;

                if (!stream.readUInt16(version))
                    return;

                if (!stream.readUInt16(numEncodings))
                    return;

                // cmap table version is always zero.
                if (version != 0)
                    return;

                // Each EncodingRecord is exactly 8 bytes:
                //
                //   uint16   platformID
                //   uint16   encodingID
                //   Offset32 subtableOffset
                if (numEncodings > stream.remaining() / 8)
                    return;

                fCmapEncodings.reserve(numEncodings);

                UnicodeCoverageBuilder coverageBuilder;

                for (uint16_t i = 0; i < numEncodings; ++i)
                {
                    CmapEncoding encoding{};

                    if (!stream.readUInt16(encoding.platformId))
                        return;

                    if (!stream.readUInt16(encoding.encodingId))
                        return;

                    if (!stream.readOffset32(encoding.offset))
                        return;

                    // EncodingRecord.subtableOffset is relative to the
                    // beginning of the cmap table.
                    auto subtable =
                        stream.subStream(encoding.offset);

                    if (!subtable.isValid())
                        continue;

                    if (!subtable.readUInt16(encoding.format))
                        continue;

                    size_t minimumHeaderSize = 0;

                    switch (encoding.format)
                    {
                        // --------------------------------------------------------
                        // 16-bit length cmap formats.
                        // --------------------------------------------------------
                    case 0:
                    case 2:
                    case 4:
                    case 6:
                    {
                        uint16_t length = 0;
                        uint16_t language = 0;

                        if (!subtable.readUInt16(length))
                            continue;

                        if (!subtable.readUInt16(language))
                            continue;

                        encoding.length = length;
                        encoding.language = language;

                        minimumHeaderSize = 6;
                        break;
                    }

                    // --------------------------------------------------------
                    // 32-bit length cmap formats.
                    // --------------------------------------------------------
                    case 8:
                    case 10:
                    case 12:
                    case 13:
                    {
                        uint16_t reserved = 0;

                        if (!subtable.readUInt16(reserved))
                            continue;

                        if (reserved != 0)
                            continue;

                        if (!subtable.readUInt32(encoding.length))
                            continue;

                        if (!subtable.readUInt32(encoding.language))
                            continue;

                        minimumHeaderSize = 12;
                        break;
                    }

                    // --------------------------------------------------------
                    // Format 14 has no language field.
                    // --------------------------------------------------------
                    case 14:
                    {
                        uint32_t numVarSelectorRecords = 0;

                        if (!subtable.readUInt32(encoding.length))
                            continue;

                        if (!subtable.readUInt32(
                            numVarSelectorRecords))
                            continue;

                        encoding.language = 0;

                        minimumHeaderSize = 10;
                        break;
                    }

                    default:
                        continue;
                    }

                    if (encoding.length < minimumHeaderSize)
                        continue;

                    // Create a bounded stream containing exactly this cmap subtable.
                    //
                    // Importantly, all subsequent coverage parsing is constrained
                    // to the subtable's declared length.
                    auto boundedSubtable =
                        stream.subStream(
                            encoding.offset,
                            encoding.length);

                    if (!boundedSubtable.isValid())
                        continue;

                    // Accumulate Unicode coverage before moving the encoding.
                    parseCmapCoverage(
                        boundedSubtable,
                        encoding.format,
                        coverageBuilder);

                    const uint32_t key =
                        makeCmapKey(
                            encoding.platformId,
                            encoding.encodingId);

                    fCmapEncodings.put(
                        key,
                        std::move(encoding));
                }

                // Produce the compact immutable representation retained by the face.
                fUnicodeCoverage = coverageBuilder.finalize();
            }

            // ==============================================
            // parseNameTable
            // ==============================================
            inline void parseNameTable(const TableRecord& table) noexcept
            {
                const ByteSpan& data = table.data;
                if (data.size() < 6)
                    return;

                OpenTypeByteStream stream(data);

                uint16_t format;
                if (!stream.readUInt16(format))
                    return;

                uint16_t count;
                if (!stream.readUInt16(count))
                    return;

                uint16_t stringOffset;
                if (!stream.readOffset16(stringOffset))
                    return;

                // stringOffset is relative to the beginning of the 'name' table.
                auto stringStorage = stream.subStream(stringOffset);
                if (!stringStorage.isValid())
                    return;

                fNameStrings.reserve(count * 4);

                for (uint16_t i = 0; i < count; ++i)
                {
                    uint16_t platformId;
                    uint16_t encodingId;
                    uint16_t languageId;
                    uint16_t nameId;
                    uint16_t length;
                    uint16_t offset;

                    if (!stream.readUInt16(platformId)) break;
                    if (!stream.readUInt16(encodingId)) break;
                    if (!stream.readUInt16(languageId)) break;
                    if (!stream.readUInt16(nameId)) break;
                    if (!stream.readUInt16(length)) break;
                    if (!stream.readOffset16(offset)) break;

                    // NameRecord.offset is relative to the beginning
                    // of the string storage area.
                    auto nameData = stringStorage.subStream(offset, length);
                    if (!nameData.isValid())
                        continue;

                    ByteSpan nameSpan = nameData.remainingData();

                    const char* internedName = nullptr;

                    if (platformId == PlatformIDs::PID_WINDOWS &&
                        (encodingId == EncodingIDs::Windows::EID_UNICODE_BMP ||
                            encodingId == EncodingIDs::Windows::EID_UNICODE_FULL))
                    {
                        std::string utf8 = convertUtf16BeToUtf8(nameSpan);
                        internedName = WSNameSet::INTERN(utf8.c_str());
                    }
                    else if (platformId == PlatformIDs::PID_UNICODE)
                    {
                        std::string utf8 = convertUtf16BeToUtf8(nameSpan);
                        internedName = WSNameSet::INTERN(utf8.c_str());
                    }
                    else
                    {
                        internedName = WSNameSet::INTERN(nameSpan);
                    }

                    if (internedName)
                    {
                        auto key = makeFullNameKey(
                            platformId,
                            encodingId,
                            languageId,
                            nameId);

                        fNameStrings.put(key, internedName);
                    }
                }
            }

            // ========================================================================
            // Name Table Access
            // ========================================================================

            inline const char* getNameString(uint16_t nameId) const noexcept {
                // Predefined fallback order (most preferred first)
                struct Candidate {
                    uint16_t platform;
                    uint16_t encoding;
                    uint16_t language;
                };

                static const Candidate candidates[] = {
                    // 1. Windows Unicode BMP, English (United States)
                    {PlatformIDs::PID_WINDOWS, EncodingIDs::Windows::EID_UNICODE_BMP, 0x0409},
                    // 2. Windows Unicode BMP, Neutral (language 0)
                    {PlatformIDs::PID_WINDOWS, EncodingIDs::Windows::EID_UNICODE_BMP, 0x0000},
                    // 3. Windows Unicode Full, English
                    {PlatformIDs::PID_WINDOWS, EncodingIDs::Windows::EID_UNICODE_FULL, 0x0409},
                    // 4. Windows Unicode Full, Neutral
                    {PlatformIDs::PID_WINDOWS, EncodingIDs::Windows::EID_UNICODE_FULL, 0x0000},
                    // 5. Unicode platform (any encoding, usually default 0)
                    {PlatformIDs::PID_UNICODE, 0, 0},
                    // 6. Macintosh (MacRoman), English
                    {PlatformIDs::PID_MACINTOSH, 0, 0},
                    // 7. Any other platform (fallback) - we can scan all, but that's slow.
                    // We'll just return nullptr if none of the above match.
                };

                for (const auto& cand : candidates) {
                    uint64_t key = makeFullNameKey(cand.platform, cand.encoding, cand.language, nameId);
                    const char* const* result = fNameStrings.getRef(key);
                    if (result && *result) {
                        return *result;
                    }
                }

                // Final fallback: try to find any record with this nameId by scanning the map.
                // This is O(N) but only if no preferred record is found.
                const char* found = nullptr;
                fNameStrings.forEach([&](uint64_t key, const char* value)
                    {
                        if (!found)
                        {
                            uint16_t storedNameId = static_cast<uint16_t>(key & 0xFFFFu);
                            if (storedNameId == nameId) {
                                found = value;
                            }
                        }
                    });

                return found;
            }

            inline const char* getNameStringEx(uint16_t platformId, uint16_t encodingId, uint16_t languageId, uint16_t nameId) const noexcept {
                auto key = makeFullNameKey(platformId, encodingId, languageId, nameId);
                const char* const* result = fNameStrings.getRef(key);

                return result ? *result : nullptr;
            }

        };


    }
}