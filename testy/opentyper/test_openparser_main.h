#pragma once

// test_opentype_full.cpp
// Comprehensive test for OpenType parser using FontMonger
//
// Usage:
//   test_opentype_full [-v] <fontfile.ttf|fontfile.otf|fontfile.ttc>
//   test_opentype_full -d [-v] <directory>
//   -v, --verbose  Show detailed table data
//   -d, --dir      Load all fonts from directory (recursive)

#include "opentype_parser.h"
#include "font_directory_view.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include <cstring>
#include <cctype>

using namespace waavs;
using namespace waavs::opentype;

// Create an INTERNed path key
const char* normalizePath(const char* path)
{
    if (!path || !*path) return nullptr;

    fs::path p(path);
    fs::path absPath = fs::absolute(p);
    std::string absStr = absPath.string();
    return WSNameSet::INTERN(absStr.c_str());
}

// ============================================================================
// Dump a hex view of a ByteSpan
// ============================================================================

void dumpHex(const ByteSpan& data, size_t maxBytes = 256) {
    size_t displaySize = std::min(data.size(), maxBytes);
    const uint8_t* p = data.begin();

    for (size_t i = 0; i < displaySize; ++i) {
        std::cout << std::hex << std::setfill('0') << std::setw(2)
            << static_cast<int>(p[i]) << " ";
        if ((i + 1) % 16 == 0) {
            std::cout << "  ";
            for (size_t j = i - 15; j <= i; ++j) {
                char c = static_cast<char>(p[j]);
                std::cout << (c >= 32 && c < 127 ? c : '.');
            }
            std::cout << std::dec << std::endl;
        }
    }

    if (displaySize % 16 != 0) {
        size_t remaining = 16 - (displaySize % 16);
        for (size_t i = 0; i < remaining * 3; ++i) {
            std::cout << " ";
        }
        std::cout << "  ";
        size_t start = (displaySize / 16) * 16;
        for (size_t j = start; j < displaySize; ++j) {
            char c = static_cast<char>(p[j]);
            std::cout << (c >= 32 && c < 127 ? c : '.');
        }
        std::cout << std::dec << std::endl;
    }

    if (data.size() > maxBytes) {
        std::cout << "... (" << (data.size() - maxBytes) << " more bytes)" << std::endl;
    }
}



// ============================================================================
// Format a tag as 4-character string
// ============================================================================

std::string tagStr(const char* tag) {
    if (!tag) return "----";
    return std::string(tag, 4);
}

std::string tagStr(Tag tag) {
    char str[5];
    str[0] = static_cast<char>((tag >> 24) & 0xFF);
    str[1] = static_cast<char>((tag >> 16) & 0xFF);
    str[2] = static_cast<char>((tag >> 8) & 0xFF);
    str[3] = static_cast<char>(tag & 0xFF);
    str[4] = '\0';
    return std::string(str, 4);
}

// ============================================================================
// Count features (common + extended) without parser.featureCount()
// ============================================================================

size_t countFeatures(const OpenTypeParser& parser) {
    size_t n = 0;
    parser.forEachFeature([&](Tag) { ++n; });
    return n;
}

// ============================================================================
// Display Table Information
// ============================================================================

void displayTableInfo(const TableRecord& record, bool verbose = false) {
    std::cout << "  Table: " << tagStr(record.tag) << std::endl;
    std::cout << "    Offset: 0x" << std::hex << record.offset << std::dec << std::endl;
    std::cout << "    Length: " << std::dec << record.length << " bytes" << std::endl;
    std::cout << "    Checksum: 0x" << std::hex << record.checksum << std::dec << std::endl;

    if (verbose && record.data.size() > 0) {
        std::cout << "    Data (first 64 bytes):" << std::endl;
        dumpHex(record.data, 64);
    }
}

void displayNameTable(const OpenTypeParser& parser) {
    std::cout << "\n=== Name Table Contents ===" << std::endl;

    struct NameInfo {
        uint16_t id;
        const char* label;
    };

    static const NameInfo nameMap[] = {
        {NameIDs::NID_COPYRIGHT, "Copyright"},
        {NameIDs::NID_FAMILY, "Family"},
        {NameIDs::NID_SUBFAMILY, "Subfamily"},
        {NameIDs::NID_UNIQUE_ID, "Unique ID"},
        {NameIDs::NID_FULL_NAME, "Full Name"},
        {NameIDs::NID_VERSION, "Version"},
        {NameIDs::NID_POSTSCRIPT, "PostScript"},
        {NameIDs::NID_TRADEMARK, "Trademark"},
        {NameIDs::NID_MANUFACTURER, "Manufacturer"},
        {NameIDs::NID_DESIGNER, "Designer"},
        {NameIDs::NID_DESCRIPTION, "Description"},
        {NameIDs::NID_VENDOR_URL, "Vendor URL"},
        {NameIDs::NID_DESIGNER_URL, "Designer URL"},
        {NameIDs::NID_LICENSE, "License"},
        {NameIDs::NID_LICENSE_URL, "License URL"},
    };

    std::cout << std::setfill(' ');

    for (const auto& entry : nameMap) {
        const char* value = parser.getNameString(entry.id);
        if (value) {
            std::cout << "  "
                << std::setw(15) << entry.label
                << ": " << value
                << std::endl;
        }
    }
}

void displayFeatureInfo(const OpenTypeParser& parser) {
    std::cout << "\n=== Features ===" << std::endl;
    size_t total = countFeatures(parser);
    std::cout << "  Total features: " << total << std::endl;

    if (total > 0) {
        std::cout << "  Features:" << std::endl;
        parser.forEachFeature([](Tag tag) {
            std::cout << "    " << tagStr(tag) << std::endl;
            });
    }
    else {
        std::cout << "  (No features detected)" << std::endl;
    }
}

void displayCmapInfo(const OpenTypeParser& parser) {
    std::cout << "\n=== CMAP Samples ===" << std::endl;

    uint32_t samples[] = {
        0x0041, // 'A'
        0x0042, // 'B'
        0x0061, // 'a'
        0x0020, // space
        0x0030, // '0'
        0x00A9, // copyright
        0x20AC, // euro
        0x1F600 // smiley emoji
    };

    for (uint32_t cp : samples) {
        uint32_t gid = parser.glyphIndex(cp);
        std::cout << "  U+" << std::hex << std::setfill('0') << std::setw(4)
            << cp << std::dec << " -> glyph " << gid << std::endl;
    }
}

void displayMetricsInfo(const OpenTypeParser& parser) {
    std::cout << "\n=== Glyph Metrics Samples ===" << std::endl;

    uint16_t glyphs[] = { 0, 1, 2, 65, 66, 97 }; // .notdef, some common glyphs

    for (uint16_t gid : glyphs) {
        if (gid < parser.getGlyphCount()) {
            auto metrics = parser.getGlyphMetrics(gid);
            std::cout << "  Glyph " << gid << ": "
                << "advance=" << metrics.advanceWidth
                << ", lsb=" << metrics.leftSideBearing
                << ", bbox=[" << metrics.xMin << "," << metrics.yMin
                << " - " << metrics.xMax << "," << metrics.yMax << "]"
                << std::endl;
        }
    }
}


// ============================================================================
// Display COLR Table
// ============================================================================

void displayCOLRInfo(
    const OpenTypeParser& parser,
    size_t maxGlyphs = 20)
{
    std::cout << "\n=== COLR Table ===" << std::endl;

    if (!parser.hasTable(TagConstants::COLR))
    {
        std::cout << "  COLR table not present" << std::endl;
        return;
    }

    if (!parser.hasCOLR())
    {
        std::cout
            << "  [FAIL] COLR table present but could not be parsed"
            << std::endl;
        return;
    }

    const auto& colr = parser.colrTable();

    std::cout << "  [PASS] COLR table parsed" << std::endl;
    std::cout << "  Version: " << colr.version << std::endl;

    // ====================================================================
    // COLR v0 / legacy-compatible records
    // ====================================================================

    std::cout
        << "  Legacy base glyphs:   "
        << colr.colorGlyphs.size()
        << std::endl;

    std::cout
        << "  Legacy layer records: "
        << colr.layers.size()
        << std::endl;

    // --------------------------------------------------------------------
    // Validate legacy BaseGlyphRecord / LayerRecord structures.
    // --------------------------------------------------------------------

    bool legacyValid = true;
    size_t referencedLayers = 0;

    uint16_t previousGlyphId = 0;
    bool havePreviousGlyph = false;

    for (const auto& glyph : colr.colorGlyphs)
    {
        if (glyph.glyphId >= parser.getGlyphCount())
        {
            std::cout
                << "  [FAIL] Base glyph out of range: "
                << glyph.glyphId
                << std::endl;

            legacyValid = false;
        }

        if (havePreviousGlyph &&
            glyph.glyphId <= previousGlyphId)
        {
            std::cout
                << "  [FAIL] Base glyph records not sorted: "
                << previousGlyphId
                << " followed by "
                << glyph.glyphId
                << std::endl;

            legacyValid = false;
        }

        previousGlyphId = glyph.glyphId;
        havePreviousGlyph = true;

        if (glyph.firstLayerIndex > colr.layers.size())
        {
            std::cout
                << "  [FAIL] Invalid firstLayerIndex for glyph "
                << glyph.glyphId
                << std::endl;

            legacyValid = false;
            continue;
        }

        if (glyph.layerCount >
            colr.layers.size() - glyph.firstLayerIndex)
        {
            std::cout
                << "  [FAIL] Invalid layer range for glyph "
                << glyph.glyphId
                << std::endl;

            legacyValid = false;
            continue;
        }

        referencedLayers += glyph.layerCount;
    }

    for (size_t i = 0; i < colr.layers.size(); ++i)
    {
        const auto& layer = colr.layers[i];

        if (layer.glyphId >= parser.getGlyphCount())
        {
            std::cout
                << "  [FAIL] Layer record "
                << i
                << " references invalid glyph "
                << layer.glyphId
                << std::endl;

            legacyValid = false;
        }
    }

    if (!colr.colorGlyphs.empty() ||
        !colr.layers.empty())
    {
        std::cout
            << "  Referenced layers:    "
            << referencedLayers
            << std::endl;

        std::cout
            << "  Legacy structure:     "
            << (legacyValid ? "[PASS]" : "[FAIL]")
            << std::endl;
    }

    // ====================================================================
    // COLR v1
    // ====================================================================

    if (colr.version == 1)
    {
        std::cout
            << "\n  --- COLRv1 ---"
            << std::endl;

        std::cout
            << "  BaseGlyphList offset:     0x"
            << std::hex
            << colr.baseGlyphListOffset
            << std::dec
            << std::endl;

        std::cout
            << "  LayerList offset:         0x"
            << std::hex
            << colr.layerListOffset
            << std::dec
            << std::endl;

        std::cout
            << "  ClipList offset:          0x"
            << std::hex
            << colr.clipListOffset
            << std::dec
            << std::endl;

        std::cout
            << "  VarIndexMap offset:       0x"
            << std::hex
            << colr.varIndexMapOffset
            << std::dec
            << std::endl;

        std::cout
            << "  VariationStore offset:    0x"
            << std::hex
            << colr.itemVariationStoreOffset
            << std::dec
            << std::endl;

        std::cout
            << "  Base glyph paint records: "
            << colr.baseGlyphPaintRecords.size()
            << std::endl;

        // ------------------------------------------------------------
        // Validate sorted glyph IDs and paint references.
        // ------------------------------------------------------------

        bool v1Valid = true;

        previousGlyphId = 0;
        havePreviousGlyph = false;

        for (const auto& record :
            colr.baseGlyphPaintRecords)
        {
            if (record.glyphId >= parser.getGlyphCount())
            {
                std::cout
                    << "  [FAIL] COLRv1 base glyph out of range: "
                    << record.glyphId
                    << std::endl;

                v1Valid = false;
            }

            if (havePreviousGlyph &&
                record.glyphId <= previousGlyphId)
            {
                std::cout
                    << "  [FAIL] COLRv1 records not sorted: "
                    << previousGlyphId
                    << " followed by "
                    << record.glyphId
                    << std::endl;

                v1Valid = false;
            }

            previousGlyphId = record.glyphId;
            havePreviousGlyph = true;

            if (record.paintOffset == 0)
            {
                std::cout
                    << "  [FAIL] Zero paint offset for glyph "
                    << record.glyphId
                    << std::endl;

                v1Valid = false;
            }
        }

        std::cout
            << "  COLRv1 structure:         "
            << (v1Valid ? "[PASS]" : "[FAIL]")
            << std::endl;

        // ------------------------------------------------------------
        // Show some BaseGlyphPaintRecords.
        // ------------------------------------------------------------

        size_t displayCount =
            std::min(
                maxGlyphs,
                colr.baseGlyphPaintRecords.size());

        for (size_t i = 0; i < displayCount; ++i)
        {
            const auto& record =
                colr.baseGlyphPaintRecords[i];

            std::cout
                << "    glyph="
                << record.glyphId
                << " paintOffset=0x"
                << std::hex
                << record.paintOffset
                << std::dec;

            // If getBaseGlyphPaint() has been added, we can
            // inspect the first byte: every Paint table begins
            // with its uint8 format.
            auto paint =
                parser.getBaseGlyphPaint(record.glyphId);

            if (paint.isValid())
            {
                uint8_t format = 0;

                if (paint.readUInt8(format))
                {
                    std::cout
                        << " format="
                        << static_cast<unsigned>(format);

                    // PaintColrLayers
                    if (format == 1)
                    {
                        uint8_t numLayers = 0;
                        uint32_t firstLayerIndex = 0;

                        if (paint.readUInt8(numLayers) &&
                            paint.readUInt32(firstLayerIndex))
                        {
                            std::cout
                                << " layers="
                                << static_cast<unsigned>(numLayers)
                                << " firstLayer="
                                << firstLayerIndex;
                        }
                    }
                }
                else
                {
                    std::cout << " format=?";
                }
            }
            else
            {
                std::cout << " [invalid paint]";
            }

            std::cout << std::endl;
        }

        if (displayCount <
            colr.baseGlyphPaintRecords.size())
        {
            std::cout
                << "    ... "
                << (colr.baseGlyphPaintRecords.size() -
                    displayCount)
                << " more COLRv1 base glyphs"
                << std::endl;
        }
    }

    // ====================================================================
    // Show some legacy v0 glyph/layer mappings
    // ====================================================================

    if (!colr.colorGlyphs.empty())
    {
        std::cout
            << "\n  --- Legacy Layer Glyphs ---"
            << std::endl;

        const size_t displayCount =
            std::min(
                maxGlyphs,
                colr.colorGlyphs.size());

        for (size_t i = 0; i < displayCount; ++i)
        {
            const auto& glyph =
                colr.colorGlyphs[i];

            std::cout
                << "  Base glyph "
                << glyph.glyphId
                << " layers="
                << glyph.layerCount
                << " firstLayer="
                << glyph.firstLayerIndex
                << std::endl;

            if (glyph.firstLayerIndex >
                colr.layers.size())
            {
                continue;
            }

            if (glyph.layerCount >
                colr.layers.size() -
                glyph.firstLayerIndex)
            {
                continue;
            }

            for (uint16_t j = 0;
                j < glyph.layerCount;
                ++j)
            {
                const size_t layerIndex =
                    static_cast<size_t>(
                        glyph.firstLayerIndex) + j;

                const auto& layer =
                    colr.layers[layerIndex];

                std::cout
                    << "    [" << j << "]"
                    << " layerRecord="
                    << layerIndex
                    << " glyph="
                    << layer.glyphId
                    << " palette=";

                if (layer.paletteIndex == 0xFFFF)
                {
                    std::cout << "foreground";
                }
                else
                {
                    std::cout
                        << layer.paletteIndex;
                }

                std::cout << std::endl;
            }
        }

        if (displayCount <
            colr.colorGlyphs.size())
        {
            std::cout
                << "\n  ... "
                << (colr.colorGlyphs.size() -
                    displayCount)
                << " more legacy color glyphs"
                << std::endl;
        }
    }
}


// ============================================================================
// Display CPAL Table
// ============================================================================

void displayCPALInfo(
    const OpenTypeParser& parser,
    size_t maxPalettes = 8,
    size_t maxEntries = 16)
{
    std::cout << "\n=== CPAL Table ===" << std::endl;

    if (!parser.hasTable(TagConstants::CPAL))
    {
        std::cout << "  CPAL table not present" << std::endl;
        return;
    }

    if (!parser.hasCPAL())
    {
        std::cout
            << "  [FAIL] CPAL table present but could not be parsed"
            << std::endl;
        return;
    }

    const auto& cpal = parser.cpalTable();

    const size_t numPalettes =
        cpal.paletteIndices.size();

    const size_t numColorRecords =
        cpal.colors.size();

    std::cout << "  [PASS] CPAL table parsed" << std::endl;
    std::cout << "  Version:             "
        << cpal.version << std::endl;

    std::cout << "  Palettes:            "
        << numPalettes << std::endl;

    std::cout << "  Entries per palette: "
        << cpal.numPaletteEntries << std::endl;

    std::cout << "  Color records:       "
        << numColorRecords << std::endl;

    // ====================================================================
    // Structural validation
    // ====================================================================

    bool valid = true;

    for (size_t i = 0; i < numPalettes; ++i)
    {
        const size_t first =
            cpal.paletteIndices[i];

        if (first > numColorRecords)
        {
            std::cout
                << "  [FAIL] Palette "
                << i
                << " starts beyond ColorRecord array: "
                << first
                << std::endl;

            valid = false;
            continue;
        }

        if (cpal.numPaletteEntries >
            numColorRecords - first)
        {
            std::cout
                << "  [FAIL] Palette "
                << i
                << " exceeds ColorRecord array"
                << std::endl;

            valid = false;
        }
    }

    // CPAL v1 arrays, when present, have fixed expected counts.

    if (!cpal.paletteTypes.empty() &&
        cpal.paletteTypes.size() != numPalettes)
    {
        std::cout
            << "  [FAIL] paletteTypes count mismatch"
            << std::endl;

        valid = false;
    }

    if (!cpal.paletteLabels.empty() &&
        cpal.paletteLabels.size() != numPalettes)
    {
        std::cout
            << "  [FAIL] paletteLabels count mismatch"
            << std::endl;

        valid = false;
    }

    if (!cpal.paletteEntryLabels.empty() &&
        cpal.paletteEntryLabels.size() !=
        cpal.numPaletteEntries)
    {
        std::cout
            << "  [FAIL] paletteEntryLabels count mismatch"
            << std::endl;

        valid = false;
    }

    std::cout
        << "  Structural check:    "
        << (valid ? "[PASS]" : "[FAIL]")
        << std::endl;

    // ====================================================================
    // CPAL v1 metadata
    // ====================================================================

    if (cpal.version == 1)
    {
        std::cout
            << "\n  --- CPAL v1 Metadata ---"
            << std::endl;

        std::cout
            << "  Palette types:       "
            << cpal.paletteTypes.size()
            << std::endl;

        std::cout
            << "  Palette labels:      "
            << cpal.paletteLabels.size()
            << std::endl;

        std::cout
            << "  Entry labels:        "
            << cpal.paletteEntryLabels.size()
            << std::endl;
    }

    // ====================================================================
    // Display palettes
    // ====================================================================

    const size_t paletteDisplayCount =
        std::min(maxPalettes, numPalettes);

    for (size_t paletteIndex = 0;
        paletteIndex < paletteDisplayCount;
        ++paletteIndex)
    {
        const size_t first =
            cpal.paletteIndices[paletteIndex];

        std::cout
            << "\n  Palette "
            << paletteIndex
            << "  firstColorRecord="
            << first;

        // ------------------------------------------------------------
        // CPAL v1 palette type flags.
        // ------------------------------------------------------------

        if (paletteIndex < cpal.paletteTypes.size())
        {
            const uint32_t flags =
                cpal.paletteTypes[paletteIndex];

            std::cout
                << "  type=0x"
                << std::hex
                << flags
                << std::dec;

            if (flags != 0)
            {
                std::cout << " [";

                bool needComma = false;

                if (flags & 0x00000001)
                {
                    std::cout << "light-bg";
                    needComma = true;
                }

                if (flags & 0x00000002)
                {
                    if (needComma)
                        std::cout << ", ";

                    std::cout << "dark-bg";
                }

                std::cout << "]";
            }
        }

        // ------------------------------------------------------------
        // CPAL v1 palette label.
        // ------------------------------------------------------------

        if (paletteIndex < cpal.paletteLabels.size())
        {
            const uint16_t nameId =
                cpal.paletteLabels[paletteIndex];

            if (nameId != 0xFFFF)
            {
                std::cout
                    << "  labelId="
                    << nameId;

                const char* label =
                    parser.getNameString(nameId);

                if (label)
                {
                    std::cout
                        << " \""
                        << label
                        << "\"";
                }
            }
        }

        std::cout << std::endl;

        if (first > numColorRecords)
            continue;

        const size_t available =
            numColorRecords - first;

        size_t entryDisplayCount =
            std::min<size_t>(
                cpal.numPaletteEntries,
                available);

        entryDisplayCount =
            std::min(
                entryDisplayCount,
                maxEntries);

        for (size_t entry = 0;
            entry < entryDisplayCount;
            ++entry)
        {
            const size_t colorIndex =
                first + entry;

            const auto& color =
                cpal.colors[colorIndex];

            std::cout
                << "    ["
                << std::setw(3)
                << entry
                << "]"
                << " record="
                << std::setw(5)
                << colorIndex
                << "  RGBA=#"

                << std::hex
                << std::setfill('0')

                << std::setw(2)
                << static_cast<unsigned>(color.red)

                << std::setw(2)
                << static_cast<unsigned>(color.green)

                << std::setw(2)
                << static_cast<unsigned>(color.blue)

                << std::setw(2)
                << static_cast<unsigned>(color.alpha)

                << std::dec
                << std::setfill(' ');

            // --------------------------------------------------------
            // CPAL v1 palette-entry label.
            // --------------------------------------------------------

            if (entry < cpal.paletteEntryLabels.size())
            {
                const uint16_t nameId =
                    cpal.paletteEntryLabels[entry];

                if (nameId != 0xFFFF)
                {
                    std::cout
                        << "  labelId="
                        << nameId;

                    const char* label =
                        parser.getNameString(nameId);

                    if (label)
                    {
                        std::cout
                            << " \""
                            << label
                            << "\"";
                    }
                }
            }

            std::cout << std::endl;
        }

        if (cpal.numPaletteEntries >
            entryDisplayCount)
        {
            std::cout
                << "    ... "
                << (cpal.numPaletteEntries -
                    entryDisplayCount)
                << " more palette entries"
                << std::endl;
        }
    }

    if (numPalettes > paletteDisplayCount)
    {
        std::cout
            << "\n  ... "
            << (numPalettes - paletteDisplayCount)
            << " more palettes"
            << std::endl;
    }
}

// ============================================================================
// Table Directory Summary (using forEachTable)
// ============================================================================

void displayTableDirectory(const OpenTypeParser& parser) {
    std::cout << "\n=== Table Directory ===" << std::endl;

    // Count the total number of tables
    size_t count = 0;
    parser.forEachTable([&](Tag, const TableRecord&)
        { ++count; });

    std::cout << "  Total tables: " << count << std::endl;
    std::cout << "  Table list:" << std::endl;

    // Enumerate each table, printing out some 
    // metadata (tag, offset, length, checksum)
    parser.forEachTable([](Tag tag, const TableRecord& record) {
        std::cout << "    " << tagStr(tag)
            << " @ 0x" << std::hex << record.offset
            << " (" << std::dec << record.length << " bytes)"
            << std::hex << " checksum=0x" << record.checksum
            << std::dec << std::endl;
        });
}

// ============================================================================
// Print a nice separator
// ============================================================================

void printSeparator(char c = '=', int width = 60) {
    for (int i = 0; i < width; ++i) std::cout << c;
    std::cout << std::endl;
}

// ============================================================================
// Display full font info (common for both single file and directory)
// ============================================================================

void displayFontInfo(const OpenTypeParser& parser, const char* label, bool verbose) {
    printSeparator('=', 70);
    std::cout << "Font: " << (label ? label : "unnamed") << std::endl;
    printSeparator('=', 70);

    if (!parser.isValid()) {
        std::cerr << "[FAIL] Invalid font!" << std::endl;
        return;
    }

    // Basic Information
    std::cout << "\n=== Basic Information ===" << std::endl;
    std::cout << "  Font Family: " << parser.getFontFamily() << std::endl;
    std::cout << "  Font Subfamily: " << parser.getFontSubfamily() << std::endl;
    std::cout << "  Full Name: " << parser.getFontFullName() << std::endl;
    std::cout << "  Version: " << (parser.getFontVersion() ? parser.getFontVersion() : "N/A") << std::endl;
    std::cout << "  Copyright: " << (parser.getCopyright() ? parser.getCopyright() : "N/A") << std::endl;
    std::cout << "  PostScript Name: " << (parser.getPostScriptName() ? parser.getPostScriptName() : "N/A") << std::endl;
    std::cout << "  Glyph Count: " << parser.getGlyphCount() << std::endl;
    std::cout << "  TrueType: " << (parser.isTrueType() ? "Yes" : "No (CFF/PostScript)") << std::endl;
    std::cout << "  Collection: " << (parser.isCollection() ? "Yes" : "No") << std::endl;

    if (parser.isCollection()) {
        const auto& header = parser.collectionHeader();
        std::cout << "    Collection fonts: " << header.numFonts << std::endl;
        std::cout << "    Collection version: 0x" << std::hex << header.version << std::dec << std::endl;
    }

    // Feature Flags
    std::cout << "\n=== Feature Flags ===" << std::endl;
    std::cout << "  Feature mask: 0x" << std::hex << parser.featureFlags() << std::dec << std::endl;

    struct FlagInfo {
        uint32_t flag;
        const char* name;
    };

    const FlagInfo flagMap[] = {
        {FeatureFlags::SVG, "SVG"},
        {FeatureFlags::COLR, "COLR"},
        {FeatureFlags::CPAL, "CPAL"},
        {FeatureFlags::CBDT, "CBDT"},
        {FeatureFlags::CBLC, "CBLC"},
        {FeatureFlags::GSUB, "GSUB"},
        {FeatureFlags::GPOS, "GPOS"},
        {FeatureFlags::KERN, "KERN"}
    };

    for (const auto& info : flagMap) {
        bool present = (parser.featureFlags() & info.flag) != 0;
        std::cout << "  " << (present ? "[PASS]" : "[FAIL]") << " " << info.name << std::endl;
    }

    // Table Directory
    displayTableDirectory(parser);

    // Detailed Table Information (verbose mode)
    if (verbose) {
        std::cout << "\n=== Detailed Table Data ===" << std::endl;
        parser.forEachTable([](Tag tag, const TableRecord& record) {
            displayTableInfo(record, true);
            });
    }

    // Name Table
    displayNameTable(parser);

    // Features
    displayFeatureInfo(parser);

    // CMAP Samples
    displayCmapInfo(parser);

    // Glyph Metrics
    displayMetricsInfo(parser);

    // Color/Emoji Support
    displayCOLRInfo(parser);
    displayCPALInfo(parser);

    // Summary
    printSeparator('-', 70);
    std::cout << "Summary:" << std::endl;
    size_t tableCount = 0;
    parser.forEachTable([&](Tag, const TableRecord&) { ++tableCount; });
    std::cout << "  Tables: " << tableCount << std::endl;
    std::cout << "  Features: " << countFeatures(parser) << std::endl;
    std::cout << "  Glyphs: " << parser.getGlyphCount() << std::endl;
    std::cout << "  Color/Emoji support: "
        << (parser.hasSVG() || parser.hasCOLR() ? "Yes" : "No") << std::endl;
    printSeparator('=', 70);
}

// ============================================================================
// Display/Test all fonts in a TTC collection
// ============================================================================

void displayCollectionFonts(
    OpenTypeParser& parser,
    const char* label,
    bool verbose)
{
    if (!parser.isCollection())
        return;

    const size_t fontCount = parser.getFontCount();

    std::cout << "\n";
    printSeparator('#', 70);
    std::cout << "TTC Collection Members: "
        << (label ? label : "unnamed")
        << std::endl;
    std::cout << "  Font count: " << fontCount << std::endl;
    printSeparator('#', 70);

    for (size_t i = 0; i < fontCount; ++i)
    {
        std::cout << "\n";
        printSeparator('-', 70);
        std::cout << "Collection Font [" << i << "]" << std::endl;
        printSeparator('-', 70);

        if (!parser.parseFont(i))
        {
            std::cout << "  [FAIL] Could not parse collection font "
                << i << std::endl;
            continue;
        }

        std::cout << "  Family:      "
            << parser.getFontFamily() << std::endl;

        std::cout << "  Subfamily:   "
            << parser.getFontSubfamily() << std::endl;

        std::cout << "  Full Name:   "
            << parser.getFontFullName() << std::endl;

        std::cout << "  PostScript:  "
            << (parser.getPostScriptName()
                ? parser.getPostScriptName()
                : "N/A")
            << std::endl;

        std::cout << "  Glyphs:      "
            << parser.getGlyphCount() << std::endl;

        std::cout << "  Outline:     "
            << (parser.isTrueType()
                ? "TrueType"
                : "CFF/PostScript")
            << std::endl;

        size_t tableCount = 0;
        parser.forEachTable(
            [&](Tag, const TableRecord&) {
                ++tableCount;
            });

        std::cout << "  Tables:      "
            << tableCount << std::endl;

        std::cout << "  Features:    "
            << countFeatures(parser) << std::endl;

        // A few useful cmap checks.
        const uint32_t gidA =
            parser.glyphIndex(0x0041);

        const uint32_t gidCJK =
            parser.glyphIndex(0x4E00);

        const uint32_t gidEmoji =
            parser.glyphIndex(0x1F600);

        std::cout << "  cmap A:      "
            << gidA << std::endl;

        std::cout << "  cmap U+4E00: "
            << gidCJK << std::endl;

        std::cout << "  cmap U+1F600:"
            << " " << gidEmoji << std::endl;

        if (verbose)
        {
            displayTableDirectory(parser);
            displayNameTable(parser);
            displayFeatureInfo(parser);
            displayCmapInfo(parser);
        }
    }

    printSeparator('#', 70);
}


// ============================================================================
// Main
// ============================================================================

int main(int argc, char* argv[]) {
    bool verbose = false;
    bool directoryMode = false;
    const char* path = nullptr;

    // Parse command line
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = true;
        }
        else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--dir") == 0) {
            directoryMode = true;
        }
        else if (argv[i][0] != '-') {
            path = argv[i];
        }
    }

    if (!path) {
        std::cerr << "Usage: " << argv[0] << " [-v|--verbose] [-d|--dir] <path>" << std::endl;
        std::cerr << "  -v, --verbose  Show detailed table data" << std::endl;
        std::cerr << "  -d, --dir      Load all fonts from directory (recursive)" << std::endl;
        std::cerr << "  path:          font file (.ttf, .otf, .ttc) or directory (with -d)" << std::endl;
        return 1;
    }

    // Create FontMonger
    FontMonger monger;

    if (directoryMode) {
        // Directory mode
        std::cout << "Loading fonts from directory: " << path << std::endl;
        size_t loaded = monger.loadDirectory(path);
        std::cout << "Loaded " << loaded << " font(s)." << std::endl;

        if (loaded == 0) {
            std::cerr << "No fonts found in directory." << std::endl;
            return 1;
        }

        // Iterate over all loaded fonts (const access)
        monger.forEach([&](const char* key, const OpenTypeParser& parser) {
            // key is the file path (interned)
            displayFontInfo(parser, key, verbose);
            });
    }
    else {
        // Single file mode
        const char* normalizedPath = normalizePath(path);
        std::cout << "Loading font from file: " << normalizedPath << std::endl;
        if (!monger.loadFontFromFile(normalizedPath)) {
            std::cerr << "Failed to load font from file." << std::endl;
            return 1;
        }

        // Retrieve the parser (const pointer, because we only read)
        OpenTypeParser* parser = monger.getFont(normalizedPath);
        if (!parser) {
            std::cerr << "Failed to retrieve parser for loaded font." << std::endl;
            return 1;
        }

        displayFontInfo(*parser, normalizedPath, verbose);

        if (parser->isCollection()) {
            displayCollectionFonts(*parser, normalizedPath, verbose);
        }
    }

    return 0;
}
