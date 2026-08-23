
// opentype_container.h
#pragma once

#include "font_face.h"
#include "opentype_bytestream.h"
#include "opentype_face.h"


namespace waavs {
    namespace opentype {

        // ============================================================
        // OpenTypeContainer
        //
        // Lazy, forward-only generator of FontFace objects from one
        // OpenType resource.
        //
        // A resource may contain:
        //
        //     TTF / OTF   -> one face
        //     TTC         -> multiple faces
        //
        // Generator contract:
        //
        //     bool operator()(FontFace& face);
        //
        // Each successful call produces the next FontFace.
        // Once exhausted, all subsequent calls return false.
        // ============================================================

        class OpenTypeContainer
        {
        private:
            SharedMemBuff fSource;
            FontName fSourceLocation{ nullptr };

            // For TTC traversal this remains positioned at the next
            // Offset32 entry in the TTC offset array.
            OpenTypeByteStream fStream;

            uint32_t fFacesRemaining{ 0 };

            bool fSingleFacePending{ false };
            bool fValid{ false };
            bool fExhausted{ false };


        public:
            OpenTypeContainer() = default;


            explicit OpenTypeContainer(
                const SharedMemBuff& source,
                FontName sourceLocation = nullptr) noexcept
                : fSource(source)
                , fSourceLocation(sourceLocation)
                , fStream(ByteSpan(
                    source.data(),
                    source.size()))
            {
                initialize();
            }


            // ========================================================
            // Generator contract
            // ========================================================

            bool operator()(FontFace& face)
            {
                face = {};

                if (!fValid || fExhausted)
                    return false;


                // ----------------------------------------------------
                // Ordinary TTF / OTF
                // ----------------------------------------------------

                if (fSingleFacePending)
                {
                    fSingleFacePending = false;
                    fExhausted = true;

                    return makeFace(
                        0,
                        face);
                }


                // ----------------------------------------------------
                // TTC
                // ----------------------------------------------------

                while (fFacesRemaining > 0)
                {
                    uint32_t faceOffset = 0;

                    if (!fStream.readOffset32(faceOffset))
                    {
                        fValid = false;
                        fExhausted = true;
                        return false;
                    }

                    --fFacesRemaining;

                    if (makeFace(
                        faceOffset,
                        face))
                    {
                        if (fFacesRemaining == 0)
                            fExhausted = true;

                        return true;
                    }

                    // A malformed face need not necessarily prevent us
                    // from trying the remaining faces in the collection.
                }


                fExhausted = true;
                return false;
            }


            // ========================================================
            // State
            // ========================================================

            bool isValid() const noexcept
            {
                return fValid;
            }


            bool exhausted() const noexcept
            {
                return fExhausted;
            }


            FontName sourceLocation() const noexcept
            {
                return fSourceLocation;
            }


        private:

            // ========================================================
            // Parse only enough of the container to establish
            // generator state.
            //
            // Individual FontFace objects are NOT created here.
            // ========================================================

            bool initialize() noexcept
            {
                if (fSource.size() < 4)
                    return false;


                Tag signature = 0;

                if (!fStream.readUInt32(signature))
                    return false;


                // ----------------------------------------------------
                // TrueType Collection
                // ----------------------------------------------------

                if (signature == TagConstants::TTCF)
                {
                    uint32_t version = 0;
                    uint32_t numFonts = 0;

                    if (!fStream.readUInt32(version))
                        return false;

                    if (!fStream.readUInt32(numFonts))
                        return false;


                    // TTC versions currently defined are:
                    //
                    //     0x00010000
                    //     0x00020000
                    //
                    if (version != 0x00010000 &&
                        version != 0x00020000)
                    {
                        return false;
                    }


                    if (numFonts == 0)
                        return false;


                    // The stream is now positioned immediately before:
                    //
                    //     Offset32 tableDirectoryOffsets[numFonts]
                    //
                    // Validate that the complete offset array is present.
                    if (numFonts >
                        fStream.remaining() / sizeof(uint32_t))
                    {
                        return false;
                    }


                    fFacesRemaining = numFonts;
                    fValid = true;

                    return true;
                }


                // ----------------------------------------------------
                // Single-face OpenType resource
                // ----------------------------------------------------

                if (!isSupportedFontContainer(signature))
                    return false;


                fSingleFacePending = true;
                fValid = true;

                return true;
            }


            // ========================================================
            // Construct one FontFace on demand.
            // ========================================================

            bool makeFace(size_t faceOffset, FontFace& face)
            {
                auto data = std::make_shared<OpenTypeFaceData>(
                        fSource,
                        faceOffset,
                        fSourceLocation);

                if (!data->isValid())
                    return false;

                face = FontFace( std::move(data));

                return true;
            }
        };

    } // namespace opentype
} // namespace waavs


/*
// opentype_parser.h


namespace waavs {
    namespace opentype {

        // =========================================================
        // OpenTypeContainer
        // =========================================================

        class OpenTypeContainer final : public IProvideFontFaces
        {
        private:
            SharedMemBuff fSource;
            FontName fSourceLocation{ nullptr };
            std::vector<FontFace> fFaces;
            bool fValid{ false };

        public:
            OpenTypeContainer() = default;

            explicit OpenTypeContainer( const SharedMemBuff& source,
                FontName srcLocation) noexcept
                : fSource(source)
                , fSourceLocation(srcLocation)
            {
                parse();
            }

            bool isValid() const noexcept
            {
                return fValid;
            }

            size_t fontFaceCount() const noexcept override
            {
                return fFaces.size();
            }

            FontFace fontFace(size_t index) const noexcept override
            {
                if (index >= fFaces.size())
                    return {};

                return fFaces[index];
            }

        private:

            inline bool parse() noexcept
            {
                ByteSpan data(fSource.data(), fSource.size());

                if (data.size() < 4)
                    return false;

                OpenTypeByteStream stream(data);

                Tag signature = 0;

                // Get the signature of the font container (first 4 bytes)
                if (!stream.readUInt32(signature))
                    return false;

                // Check if the signature indicates a TrueType Collection (TTC)
                // If it is a TTC, parse the collection; 
                if (signature == TagConstants::TTCF)
                {
                    if (!parseTTC(stream))
                        return false;
                }
                else      // otherwise, parse a single font face
                {
                    if (!isSupportedFontContainer(signature))
                        return false;

                    if (!addFace(0))
                        return false;
                }

                fValid = !fFaces.empty();
                return fValid;
            }

            inline bool parseTTC(OpenTypeByteStream& stream) noexcept
            {
                uint32_t version = 0;
                uint32_t numFonts = 0;

                if (!stream.readUInt32(version))
                    return false;

                if (!stream.readUInt32(numFonts))
                    return false;

                if (numFonts == 0 ||
                    numFonts > stream.remaining() / 4)
                {
                    return false;
                }

                fFaces.reserve(numFonts);

                for (uint32_t i = 0; i < numFonts; ++i)
                {
                    uint32_t faceOffset = 0;

                    if (!stream.readOffset32(faceOffset))
                        return false;

                    if (!addFace(faceOffset))
                        return false;
                }

                return true;
            }

            inline bool addFace(size_t faceOffset) noexcept
            {
                auto data = std::make_shared<OpenTypeFaceData>( 
                    fSource, 
                    faceOffset, 
                    fSourceLocation);

                if (!data->isValid())
                    return false;

                fFaces.emplace_back(
                    std::move(data));

                return true;
            }
        };
    }
}
*/

/*
namespace waavs {
    namespace opentype {

        // ============================================================================
        // OpenTypeParser
        // 
        // Main parser for OpenType fonts and collections.
        // Uses WSOpenHashMap for table storage and WSNameSet for string interning.
        // All methods are header-only.
        // ============================================================================

        class OpenTypeParser 
        {
        private:
            FeatureSet mFeatures;

        public:
            // ========================================================================
            // Constructors
            // ========================================================================

            OpenTypeParser() = default;

            explicit OpenTypeParser(const ByteSpan& data) noexcept
            {
                if (data.size() >= 4 &&
                    isSupportedFontContainer(OSIG(data.begin())))
                {
                    mData = data;
                    mStream = OpenTypeByteStream(data);
                    parse();
                }
            }

            explicit OpenTypeParser(const MemBuff& buffer) noexcept
            {
                ByteSpan data(buffer.data(), buffer.size());

                if (data.size() >= 4 &&
                    isSupportedFontContainer(OSIG(data.begin())))
                {
                    mData = data;
                    mStream = OpenTypeByteStream(data);
                    parse();
                }
            }

            explicit OpenTypeParser(const SharedMemBuff& buffer) noexcept
            {
                mFontData = buffer;

                ByteSpan data(buffer.data(), buffer.size());

                if (data.size() >= 4 &&
                    isSupportedFontContainer(OSIG(data.begin())))
                {
                    mData = data;
                    mStream = OpenTypeByteStream(data);
                    parse();
                }
            }

            // ========================================================================
            // Basic Information
            // ========================================================================


            inline bool isCollection() const noexcept { return mIsCollection; }
            inline const TTCHeader& collectionHeader() const noexcept { return mTtcHeader; }
            inline size_t getFontCount() const noexcept {
                return mIsCollection ? mTtcHeader.numFonts : 1;
            }
            inline uint16_t getGlyphCount() const noexcept { return mNumGlyphs; }
            inline bool isTrueType() const noexcept { return mIsTrueType; }
            inline uint32_t featureFlags() const noexcept { return mFeatureFlags; }
            inline bool supportsFeature(uint32_t feature) const noexcept {
                return (mFeatureFlags & feature) != 0;
            }

            // =====================================
            // Feature access (using FeatureSet)
            // =====================================
                // Fast path - check common feature by bit
            inline bool hasFeature(CommonFeatureBit bit) const noexcept {
                return mFeatures.hasFeature(static_cast<uint64_t>(bit));
            }

            // General check
            inline bool hasFeature(Tag tag) const noexcept {
                return mFeatures.hasFeature(tag);
            }

            // Get common mask for fast layout
            inline uint64_t featureMask() const noexcept {
                return mFeatures.commonMask();
            }

            // Iterate all features
            template<typename Fn>
            inline void forEachFeature(Fn&& fn) const noexcept {
                mFeatures.forEach(std::forward<Fn>(fn));
            }

            // ========================================================================
            // Table Access (using Tag)
            // ========================================================================
            inline const TableRecord* getTable(Tag tag) const noexcept
            {
                return mTables.getRef(tag);
            }

            inline bool hasTable(Tag tag) const noexcept
            {
                return mTables.contains(tag);
            }

            template<typename Fn>
            inline void forEachTable(Fn&& fn) const noexcept 
            {
                mTables.forEach([&fn](Tag key, const TableRecord& value) {
                    fn(key, value);
                    });
            }

            // ========================================================================
            // Name Table Access
            // ========================================================================



            inline const char* getFontFamily() const noexcept {
                const char* name = getNameString(NameIDs::NID_FAMILY);

                return name ? name : WSNameSet::INTERN("Unknown");
            }

            inline const char* getFontSubfamily() const noexcept {
                const char* name = getNameString(NameIDs::NID_SUBFAMILY);

                return name ? name : WSNameSet::INTERN("Regular");
            }

            inline const char* getFontFullName() const noexcept {
                const char* name = getNameString(NameIDs::NID_FULL_NAME);
                if (!name) 
                    name = getNameString(NameIDs::NID_FAMILY);

                return name ? name : WSNameSet::INTERN("Unknown");
            }

            inline const char* getFontVersion() const noexcept { return getNameString(NameIDs::NID_VERSION); }
            inline const char* getCopyright() const noexcept { return getNameString(NameIDs::NID_COPYRIGHT); }
            inline const char* getPostScriptName() const noexcept { return getNameString(NameIDs::NID_POSTSCRIPT); }
            inline const char* getManufacturer() const noexcept { return getNameString(NameIDs::NID_MANUFACTURER); }
            inline const char* getDesigner() const noexcept { return getNameString(NameIDs::NID_DESIGNER); }

            // ========================================================================
            // CMAP Access
            // ========================================================================

            inline uint32_t glyphIndex(uint32_t codepoint) const noexcept
            {
                uint32_t key = makeCmapKey(PlatformIDs::PID_WINDOWS, EncodingIDs::Windows::EID_UNICODE_FULL);

                const CmapEncoding* enc = mCmapEncodings.getRef(key);

                if (enc)
                {
                    uint32_t idx = glyphIndexForEncoding(codepoint, *enc);
                    if (idx != 0)
                        return idx;
                }

                // Fall back to Windows BMP.
                key = makeCmapKey(PlatformIDs::PID_WINDOWS, EncodingIDs::Windows::EID_UNICODE_BMP);
                enc = mCmapEncodings.getRef(key);

                if (enc)
                {
                    uint32_t idx = glyphIndexForEncoding(codepoint, *enc);
                    if (idx != 0)
                        return idx;
                }

                // Unicode platform fallback.
                key = makeCmapKey(PlatformIDs::PID_UNICODE, EncodingIDs::Unicode::EID_DEFAULT);
                enc = mCmapEncodings.getRef(key);

                if (enc)
                    return glyphIndexForEncoding(codepoint, *enc);

                return 0;
            }

            // ========================================================================
            // Glyph Metrics (cached in WSNameMap)
            // ========================================================================

            inline GlyphMetrics getGlyphMetrics(uint16_t glyphId) const noexcept 
            {
                GlyphMetrics metrics{};

                // Check cache
                const uint32_t cacheKey = glyphId;
                const GlyphMetrics* cached = mGlyphMetrics.getRef(cacheKey);
                if (cached) {
                    return *cached;
                }

                // Parse from tables
                const TableRecord* hmtxTable = getTable(TagConstants::HMTX);
                const TableRecord* hheaTable = getTable(TagConstants::HHEA);
                const TableRecord* glyfTable = getTable(TagConstants::GLYF);
                const TableRecord* locaTable = getTable(TagConstants::LOCA);
                const TableRecord* headTable = getTable(TagConstants::HEAD);

                if (hmtxTable && hheaTable) {
                    parseHorizontalMetrics(glyphId, *hmtxTable, *hheaTable, metrics);
                }

                if (glyfTable && locaTable && headTable) {
                    parseBoundingBox(glyphId, *glyfTable, *locaTable, *headTable, metrics);
                }

                // Cache the result
                mGlyphMetrics.put(cacheKey, metrics);

                return metrics;
            }

            // ========================================================================
            // SVG Table Access
            // ========================================================================

            inline bool hasSVG() const noexcept { return mHasSVG; }
            inline const OpenTypeSVGTable& svgTable() const noexcept { return mSVGTable; }

            inline ByteSpan getSVGForGlyph(uint16_t glyphId) const noexcept {
                if (!mHasSVG) return ByteSpan();

                for (const auto& doc : mSVGTable.documents) {
                    if (glyphId >= doc.startGlyph && glyphId <= doc.endGlyph) {
                        if (doc.decodedData) {
                            return ByteSpan(doc.decodedData.data(), doc.decodedData.size());
                        }
                        return doc.data;
                    }
                }
                return ByteSpan();
            }

            // ========================================================================
            // COLR Table Access
            // ========================================================================

            inline bool hasCOLR() const noexcept { return mHasCOLR; }
            inline const OpenTypeCOLRTable& colrTable() const noexcept { return mCOLRTable; }
            inline const OpenTypeCPALTable& cpalTable() const noexcept { return mCPALTable; }

            inline const OpenTypeColorGlyph*
                getColorGlyph(uint16_t glyphId) const noexcept
            {
                if (!mHasCOLR)
                    return nullptr;

                const auto& glyphs =
                    mCOLRTable.colorGlyphs;

                auto it = std::lower_bound(
                    glyphs.begin(),
                    glyphs.end(),
                    glyphId,
                    [](const OpenTypeColorGlyph& glyph,
                        uint16_t id) noexcept
                    {
                        return glyph.glyphId < id;
                    });

                if (it == glyphs.end() ||
                    it->glyphId != glyphId)
                {
                    return nullptr;
                }

                return &*it;
            }

            // ========================================================================
            // CPAL Table Access
            // ========================================================================

            inline bool hasCPAL() const noexcept { return mHasCPAL; }




            // ========================================================================
            // Validation
            // ========================================================================

            static inline uint32_t calcChecksum(const ByteSpan& data) noexcept {
                return OpenTypeByteStream::calcChecksum(data);
            }

        public:

        private:
            // ========================================================================
            // Data Members - 
            // ==========================================================*==============


            // SVG/COLR support
            bool mHasSVG{ false };
            bool mHasCOLR{ false };
            bool mHasCPAL{ false };
            OpenTypeSVGTable mSVGTable;
            OpenTypeCOLRTable mCOLRTable;
            OpenTypeCPALTable mCPALTable;

            // ========================================================================
            // Parsing Methods
            // ========================================================================

            inline void parseSVGTable() noexcept {
                const TableRecord* table = getTable(TagConstants::SVG);
                if (!table) return;

                mHasSVG = true;
                mFeatureFlags |= FeatureFlags::SVG;

                const ByteSpan& data = table->data;
                if (data.size() < 8) return;

                OpenTypeByteStream stream(data);

                if (!stream.readUInt16(mSVGTable.version)) return;
                stream.skip(2);
                if (!stream.readUInt32(mSVGTable.numEntries)) return;

                mSVGTable.documents.reserve(mSVGTable.numEntries);

                for (uint32_t i = 0; i < mSVGTable.numEntries; ++i) {
                    OpenTypeSVGDocument doc;
                    if (!stream.readUInt32(doc.startOffset)) break;
                    if (!stream.readUInt32(doc.endOffset)) break;
                    if (!stream.readUInt32(doc.startGlyph)) break;
                    if (!stream.readUInt32(doc.endGlyph)) break;

                    size_t dataSize = doc.endOffset - doc.startOffset;
                    if (dataSize > 0 && doc.startOffset + dataSize <= table->data.size()) {
                        doc.data = table->data.subSpan(doc.startOffset, dataSize);
                    }

                    mSVGTable.documents.push_back(std::move(doc));
                }
            }

            // -------------------------------------
            // parseCOLRTable - Full implementation for COLR v0 and v1
            // -------------------------------------

            inline bool parseCOLRBaseGlyphList(
                OpenTypeByteStream colrStream,
                uint32_t baseGlyphListOffset,
                OpenTypeCOLRTable& colr) noexcept
            {
                if (baseGlyphListOffset == 0)
                    return false;

                // baseGlyphListOffset is relative to beginning of COLR.
                auto baseGlyphList =
                    colrStream.subStream(baseGlyphListOffset);

                if (!baseGlyphList.isValid())
                    return false;

                // ------------------------------------------------------------
                // BaseGlyphList:
                //
                // uint32 numBaseGlyphPaintRecords
                //
                // BaseGlyphPaintRecord records[count]:
                //     uint16   glyphID
                //     Offset32 paintOffset
                //
                // Each record is 6 bytes.
                // ------------------------------------------------------------

                uint32_t recordCount = 0;

                if (!baseGlyphList.readUInt32(recordCount))
                    return false;

                if (recordCount > baseGlyphList.remaining() / 6)
                    return false;

                colr.baseGlyphPaintRecords.clear();
                colr.baseGlyphPaintRecords.reserve(recordCount);

                uint16_t previousGlyphId = 0;
                bool havePreviousGlyph = false;

                for (uint32_t i = 0; i < recordCount; ++i)
                {
                    OpenTypeBaseGlyphPaintRecord record{};

                    if (!baseGlyphList.readUInt16(record.glyphId))
                        return false;

                    if (!baseGlyphList.readOffset32(record.paintOffset))
                        return false;

                    // Base glyph must be a real glyph.
                    if (record.glyphId >= mNumGlyphs)
                        return false;

                    // BaseGlyphPaintRecords are required to be sorted
                    // in strictly increasing glyph-ID order.
                    if (havePreviousGlyph &&
                        record.glyphId <= previousGlyphId)
                    {
                        return false;
                    }

                    previousGlyphId = record.glyphId;
                    havePreviousGlyph = true;

                    // paintOffset is relative to BaseGlyphList -- NOT to the
                    // BaseGlyphPaintRecord and NOT to the COLR table.
                    //
                    // A zero offset cannot identify a Paint table.
                    if (record.paintOffset == 0)
                        return false;

                    auto paint =
                        baseGlyphList.subStream(record.paintOffset);

                    if (!paint.isValid() || paint.empty())
                        return false;

                    colr.baseGlyphPaintRecords.push_back(record);
                }

                return true;
            }

            inline const OpenTypeBaseGlyphPaintRecord*
                getBaseGlyphPaintRecord(uint16_t glyphId) const noexcept
            {
                if (!mHasCOLR ||
                    mCOLRTable.version != 1)
                {
                    return nullptr;
                }

                const auto& records =
                    mCOLRTable.baseGlyphPaintRecords;

                auto it = std::lower_bound(
                    records.begin(),
                    records.end(),
                    glyphId,
                    [](const OpenTypeBaseGlyphPaintRecord& record,
                        uint16_t id)
                    {
                        return record.glyphId < id;
                    });

                if (it == records.end() ||
                    it->glyphId != glyphId)
                {
                    return nullptr;
                }

                return &*it;
            }

            public:
            inline OpenTypeByteStream getBaseGlyphPaint( uint16_t glyphId) const noexcept
            {
                const OpenTypeBaseGlyphPaintRecord* record =
                    getBaseGlyphPaintRecord(glyphId);

                if (!record)
                    return {};

                const TableRecord* colrTable =
                    getTable(TagConstants::COLR);

                if (!colrTable)
                    return {};

                OpenTypeByteStream colr(colrTable->data);

                auto baseGlyphList =
                    colr.subStream(mCOLRTable.baseGlyphListOffset);

                if (!baseGlyphList.isValid())
                    return {};

                // paintOffset is based at BaseGlyphList.
                return baseGlyphList.subStream(
                    record->paintOffset);
            }

            public:
            inline const OpenTypeColorLayer*
                getColorLayers(
                    uint16_t glyphId,
                    size_t& layerCount) const noexcept
            {
                layerCount = 0;

                const OpenTypeColorGlyph* glyph =
                    getColorGlyph(glyphId);

                if (!glyph)
                    return nullptr;

                const size_t first =
                    glyph->firstLayerIndex;

                const size_t count =
                    glyph->layerCount;

                if (first > mCOLRTable.layers.size())
                    return nullptr;

                if (count >
                    mCOLRTable.layers.size() - first)
                {
                    return nullptr;
                }

                layerCount = count;

                return mCOLRTable.layers.data() + first;
            }

            inline bool validateCOLRv0() const noexcept
            {
                if (!mHasCOLR ||
                    mCOLRTable.version != 0)
                {
                    return false;
                }

                if (!mHasCPAL)
                    return false;

                for (const auto& layer : mCOLRTable.layers)
                {
                    if (layer.paletteIndex == 0xFFFF)
                        continue;

                    if (layer.paletteIndex >=
                        mCPALTable.numPaletteEntries)
                    {
                        return false;
                    }
                }

                return true;
            }

            public:
            inline bool canRenderCOLRv0() const noexcept
            {
                return validateCOLRv0();
            }

            private:
            inline bool parseCOLRLayerList(
                OpenTypeByteStream colrStream,
                uint32_t layerListOffset,
                OpenTypeCOLRTable& colr) noexcept
            {
                colr.layerPaintOffsets.clear();

                // LayerList is optional.
                if (layerListOffset == 0)
                    return true;

                // layerListOffset is relative to beginning of COLR.
                auto layerList =
                    colrStream.subStream(layerListOffset);

                if (!layerList.isValid())
                    return false;

                // ------------------------------------------------------------
                // LayerList:
                //
                // uint32   numLayers
                // Offset32 paintOffsets[numLayers]
                //
                // paintOffsets are relative to beginning of LayerList.
                // ------------------------------------------------------------

                uint32_t numLayers = 0;

                if (!layerList.readUInt32(numLayers))
                    return false;

                if (numLayers > layerList.remaining() / 4)
                    return false;

                colr.layerPaintOffsets.reserve(numLayers);

                for (uint32_t i = 0; i < numLayers; ++i)
                {
                    uint32_t paintOffset = 0;

                    if (!layerList.readOffset32(paintOffset))
                        return false;

                    if (paintOffset == 0)
                        return false;

                    // Validate that this actually resolves somewhere within
                    // the LayerList-based coordinate system.
                    auto paint =
                        layerList.subStream(paintOffset);

                    if (!paint.isValid() || paint.empty())
                        return false;

                    colr.layerPaintOffsets.push_back(paintOffset);
                }

                return true;
            }

            private:
            inline void parseCOLRTable() noexcept
            {
                const TableRecord* table =
                    getTable(TagConstants::COLR);

                if (!table)
                    return;

                OpenTypeByteStream stream(table->data);

                // ------------------------------------------------------------
                // COLR common header
                //
                // Version 0:
                //
                // uint16   version
                // uint16   numBaseGlyphRecords
                // Offset32 baseGlyphRecordsOffset
                // Offset32 layerRecordsOffset
                // uint16   numLayerRecords
                //
                // Version 1 extends this with:
                //
                // Offset32 baseGlyphListOffset
                // Offset32 layerListOffset
                // Offset32 clipListOffset
                // Offset32 varIndexMapOffset
                // Offset32 itemVariationStoreOffset
                // ------------------------------------------------------------

                uint16_t version = 0;
                uint16_t numBaseGlyphRecords = 0;
                uint32_t baseGlyphRecordsOffset = 0;
                uint32_t layerRecordsOffset = 0;
                uint16_t numLayerRecords = 0;

                if (!stream.readUInt16(version))
                    return;

                if (version > 1)
                    return;

                if (!stream.readUInt16(numBaseGlyphRecords))
                    return;

                if (!stream.readOffset32(baseGlyphRecordsOffset))
                    return;

                if (!stream.readOffset32(layerRecordsOffset))
                    return;

                if (!stream.readUInt16(numLayerRecords))
                    return;

                // Build locally. Only publish after complete successful parsing.
                OpenTypeCOLRTable colr{};
                colr.version = version;

                // ------------------------------------------------------------
                // COLR v1 extension
                // ------------------------------------------------------------

                if (version == 1)
                {
                    if (!stream.readOffset32(colr.baseGlyphListOffset))
                        return;

                    if (!stream.readOffset32(colr.layerListOffset))
                        return;

                    if (!stream.readOffset32(colr.clipListOffset))
                        return;

                    if (!stream.readOffset32(colr.varIndexMapOffset))
                        return;

                    if (!stream.readOffset32(colr.itemVariationStoreOffset))
                        return;

                    // BaseGlyphList is the primary mapping from a base glyph
                    // to the root Paint table for COLRv1.
                    if (colr.baseGlyphListOffset == 0)
                        return;

                    if (!parseCOLRBaseGlyphList(
                        stream,
                        colr.baseGlyphListOffset,
                        colr))
                    {
                        return;
                    }

                    if (!parseCOLRLayerList(
                        stream,
                        colr.layerListOffset,
                        colr))
                    {
                        return;
                    }
                }

                // ------------------------------------------------------------
                // Legacy COLR v0-compatible structures
                //
                // Required for COLR v0.
                // Optional for COLR v1.
                // ------------------------------------------------------------

                OpenTypeByteStream baseGlyphRecords;
                OpenTypeByteStream layerRecords;

                // ------------------------------------------------------------
                // BaseGlyphRecord[]
                //
                // uint16 glyphID
                // uint16 firstLayerIndex
                // uint16 numLayers
                //
                // Each record = 6 bytes.
                // ------------------------------------------------------------

                if (numBaseGlyphRecords != 0)
                {
                    if (baseGlyphRecordsOffset == 0)
                        return;

                    baseGlyphRecords =
                        stream.subStream(
                            baseGlyphRecordsOffset,
                            static_cast<size_t>(numBaseGlyphRecords) * 6);

                    if (!baseGlyphRecords.isValid())
                        return;
                }
                else
                {
                    // A v0 table is expected to use the v0 structures.
                    //
                    // For v1, zero legacy BaseGlyphRecords is perfectly valid.
                    if (version == 0)
                        return;

                    // If there are no records, the offset should normally be NULL.
                    // We don't need to reject a non-zero unused offset here.
                }

                // ------------------------------------------------------------
                // LayerRecord[]
                //
                // uint16 glyphID
                // uint16 paletteIndex
                //
                // Each record = 4 bytes.
                // ------------------------------------------------------------

                if (numLayerRecords != 0)
                {
                    if (layerRecordsOffset == 0)
                        return;

                    layerRecords =
                        stream.subStream(
                            layerRecordsOffset,
                            static_cast<size_t>(numLayerRecords) * 4);

                    if (!layerRecords.isValid())
                        return;
                }
                else
                {
                    if (version == 0)
                        return;
                }

                colr.colorGlyphs.reserve(numBaseGlyphRecords);
                colr.layers.reserve(numLayerRecords);

                // ------------------------------------------------------------
                // Parse BaseGlyphRecord array.
                //
                // Records must be sorted by glyphID.
                // ------------------------------------------------------------

                uint16_t previousGlyphId = 0;
                bool havePreviousGlyph = false;

                for (uint16_t i = 0;
                    i < numBaseGlyphRecords;
                    ++i)
                {
                    OpenTypeColorGlyph glyph{};

                    if (!baseGlyphRecords.readUInt16(glyph.glyphId))
                        return;

                    if (!baseGlyphRecords.readUInt16(glyph.firstLayerIndex))
                        return;

                    if (!baseGlyphRecords.readUInt16(glyph.layerCount))
                        return;

                    // Glyph ID must exist in the font.
                    if (glyph.glyphId >= mNumGlyphs)
                        return;

                    // BaseGlyphRecord[] must be strictly increasing by glyph ID.
                    if (havePreviousGlyph &&
                        glyph.glyphId <= previousGlyphId)
                    {
                        return;
                    }

                    previousGlyphId = glyph.glyphId;
                    havePreviousGlyph = true;

                    // Validate the referenced slice of LayerRecord[].
                    //
                    // Avoid firstLayerIndex + layerCount to prevent overflow-style
                    // range errors.
                    if (glyph.firstLayerIndex > numLayerRecords)
                        return;

                    if (glyph.layerCount >
                        static_cast<uint16_t>(
                            numLayerRecords - glyph.firstLayerIndex))
                    {
                        return;
                    }

                    colr.colorGlyphs.push_back(glyph);
                }

                // ------------------------------------------------------------
                // Parse shared LayerRecord array.
                //
                // BaseGlyphRecords refer to slices of this vector. Different
                // BaseGlyphRecords may legitimately refer to overlapping slices.
                // ------------------------------------------------------------

                for (uint16_t i = 0;
                    i < numLayerRecords;
                    ++i)
                {
                    OpenTypeColorLayer layer{};

                    if (!layerRecords.readUInt16(layer.glyphId))
                        return;

                    if (!layerRecords.readUInt16(layer.paletteIndex))
                        return;

                    if (layer.glyphId >= mNumGlyphs)
                        return;

                    // Don't validate paletteIndex until CPAL has been parsed.
                    //
                    // 0xFFFF is also special and means application foreground color.

                    colr.layers.push_back(layer);
                }

                // ------------------------------------------------------------
                // Success
                // ------------------------------------------------------------

                mCOLRTable = std::move(colr);

                mHasCOLR = true;
                mFeatureFlags |= FeatureFlags::COLR;
            }

            // ==========================================
            // getCOLRLayerPaint
            // - Returns a substream of the COLR table
            // for the specified layer index.
            //
            public:
            inline OpenTypeByteStream getCOLRLayerPaint(uint32_t layerIndex) const noexcept
            {
                if (!mHasCOLR ||
                    mCOLRTable.version != 1)
                {
                    return {};
                }

                if (layerIndex >=
                    mCOLRTable.layerPaintOffsets.size())
                {
                    return {};
                }

                const TableRecord* table =
                    getTable(TagConstants::COLR);

                if (!table)
                    return {};

                OpenTypeByteStream colr(table->data);

                auto layerList =
                    colr.subStream(
                        mCOLRTable.layerListOffset);

                if (!layerList.isValid())
                    return {};

                return layerList.subStream(
                    mCOLRTable.layerPaintOffsets[layerIndex]);
            }

            // ==========================================
            // parseCPALTable - Full implementation for CPAL v0 and v1
            // ==========================================
            public: inline const OpenTypeCPALColor*
                getPaletteColor(
                    uint16_t paletteIndex,
                    uint16_t entryIndex) const noexcept
            {
                if (!mHasCPAL)
                    return nullptr;

                if (paletteIndex >=
                    mCPALTable.paletteIndices.size())
                {
                    return nullptr;
                }

                if (entryIndex >=
                    mCPALTable.numPaletteEntries)
                {
                    return nullptr;
                }

                const size_t first =
                    mCPALTable.paletteIndices[paletteIndex];

                const size_t index =
                    first + entryIndex;

                if (index >= mCPALTable.colors.size())
                    return nullptr;

                return &mCPALTable.colors[index];
            }

            private: inline void parseCPALTable() noexcept
            {
                const TableRecord* table =
                    getTable(TagConstants::CPAL);

                if (!table)
                    return;

                OpenTypeByteStream stream(table->data);

                // ------------------------------------------------------------
                // CPAL v0 common header
                //
                // uint16   version
                // uint16   numPaletteEntries
                // uint16   numPalettes
                // uint16   numColorRecords
                // Offset32 colorRecordsArrayOffset
                // uint16   colorRecordIndices[numPalettes]
                //
                // CPAL v1 then adds:
                //
                // Offset32 paletteTypesArrayOffset
                // Offset32 paletteLabelsArrayOffset
                // Offset32 paletteEntryLabelsArrayOffset
                // ------------------------------------------------------------

                uint16_t version = 0;
                uint16_t numPaletteEntries = 0;
                uint16_t numPalettes = 0;
                uint16_t numColorRecords = 0;
                uint32_t colorRecordsArrayOffset = 0;

                if (!stream.readUInt16(version))
                    return;

                if (version > 1)
                    return;

                if (!stream.readUInt16(numPaletteEntries))
                    return;

                if (!stream.readUInt16(numPalettes))
                    return;

                if (!stream.readUInt16(numColorRecords))
                    return;

                if (!stream.readOffset32(colorRecordsArrayOffset))
                    return;

                // A CPAL table must contain at least one palette,
                // and each palette must contain at least one entry.
                if (numPalettes == 0 ||
                    numPaletteEntries == 0 ||
                    numColorRecords == 0)
                {
                    return;
                }

                // The palette-start index array follows directly.
                if (numPalettes > stream.remaining() / 2)
                    return;

                OpenTypeCPALTable cpal{};

                cpal.version = version;
                cpal.numPaletteEntries = numPaletteEntries;

                cpal.paletteIndices.reserve(numPalettes);
                cpal.colors.reserve(numColorRecords);

                // ------------------------------------------------------------
                // colorRecordIndices[]
                //
                // These are record indices, not byte offsets.
                // ------------------------------------------------------------

                for (uint16_t i = 0; i < numPalettes; ++i)
                {
                    uint16_t colorRecordIndex = 0;

                    if (!stream.readUInt16(colorRecordIndex))
                        return;

                    // Each palette needs numPaletteEntries consecutive
                    // ColorRecords beginning at colorRecordIndex.
                    if (colorRecordIndex > numColorRecords)
                        return;

                    if (numPaletteEntries >
                        static_cast<uint16_t>(
                            numColorRecords - colorRecordIndex))
                    {
                        return;
                    }

                    cpal.paletteIndices.push_back(
                        colorRecordIndex);
                }

                // ------------------------------------------------------------
                // CPAL v1 extension.
                //
                // IMPORTANT: these fields occur AFTER colorRecordIndices[].
                // ------------------------------------------------------------

                uint32_t paletteTypesArrayOffset = 0;
                uint32_t paletteLabelsArrayOffset = 0;
                uint32_t paletteEntryLabelsArrayOffset = 0;

                if (version == 1)
                {
                    if (!stream.readOffset32(paletteTypesArrayOffset))
                        return;

                    if (!stream.readOffset32(paletteLabelsArrayOffset))
                        return;

                    if (!stream.readOffset32(
                        paletteEntryLabelsArrayOffset))
                    {
                        return;
                    }
                }

                // ------------------------------------------------------------
                // ColorRecord[]
                //
                // uint8 blue
                // uint8 green
                // uint8 red
                // uint8 alpha
                //
                // colorRecordsArrayOffset is relative to CPAL.
                // ------------------------------------------------------------

                if (colorRecordsArrayOffset == 0)
                    return;

                auto colorRecords =
                    stream.subStream(
                        colorRecordsArrayOffset,
                        static_cast<size_t>(numColorRecords) * 4);

                if (!colorRecords.isValid())
                    return;

                for (uint16_t i = 0; i < numColorRecords; ++i)
                {
                    OpenTypeCPALColor color{};

                    if (!colorRecords.readUInt8(color.blue))
                        return;

                    if (!colorRecords.readUInt8(color.green))
                        return;

                    if (!colorRecords.readUInt8(color.red))
                        return;

                    if (!colorRecords.readUInt8(color.alpha))
                        return;

                    cpal.colors.push_back(color);
                }

                // ------------------------------------------------------------
                // CPAL v1 Palette Types Array
                //
                // uint32 paletteTypes[numPalettes]
                // ------------------------------------------------------------

                if (paletteTypesArrayOffset != 0)
                {
                    auto paletteTypes =
                        stream.subStream(
                            paletteTypesArrayOffset,
                            static_cast<size_t>(numPalettes) * 4);

                    if (!paletteTypes.isValid())
                        return;

                    cpal.paletteTypes.reserve(numPalettes);

                    for (uint16_t i = 0; i < numPalettes; ++i)
                    {
                        uint32_t flags = 0;

                        if (!paletteTypes.readUInt32(flags))
                            return;

                        // Bits currently defined:
                        //
                        // 0x0001 = usable with light background
                        // 0x0002 = usable with dark background
                        //
                        // Keep the raw flags rather than rejecting unknown
                        // bits, allowing future extensions.
                        cpal.paletteTypes.push_back(flags);
                    }
                }

                // ------------------------------------------------------------
                // CPAL v1 Palette Labels Array
                //
                // uint16 paletteLabels[numPalettes]
                // ------------------------------------------------------------

                if (paletteLabelsArrayOffset != 0)
                {
                    auto paletteLabels =
                        stream.subStream(
                            paletteLabelsArrayOffset,
                            static_cast<size_t>(numPalettes) * 2);

                    if (!paletteLabels.isValid())
                        return;

                    cpal.paletteLabels.reserve(numPalettes);

                    for (uint16_t i = 0; i < numPalettes; ++i)
                    {
                        uint16_t nameId = 0;

                        if (!paletteLabels.readUInt16(nameId))
                            return;

                        cpal.paletteLabels.push_back(nameId);
                    }
                }

                // ------------------------------------------------------------
                // CPAL v1 Palette Entry Labels Array
                //
                // uint16 paletteEntryLabels[numPaletteEntries]
                // ------------------------------------------------------------

                if (paletteEntryLabelsArrayOffset != 0)
                {
                    auto entryLabels =
                        stream.subStream(
                            paletteEntryLabelsArrayOffset,
                            static_cast<size_t>(numPaletteEntries) * 2);

                    if (!entryLabels.isValid())
                        return;

                    cpal.paletteEntryLabels.reserve(
                        numPaletteEntries);

                    for (uint16_t i = 0;
                        i < numPaletteEntries;
                        ++i)
                    {
                        uint16_t nameId = 0;

                        if (!entryLabels.readUInt16(nameId))
                            return;

                        cpal.paletteEntryLabels.push_back(nameId);
                    }
                }

                // ------------------------------------------------------------
                // Success
                // ------------------------------------------------------------

                mCPALTable = std::move(cpal);

                mHasCPAL = true;
                mFeatureFlags |= FeatureFlags::CPAL;
            }

            // =======================================
            // parseGSUBTable()
            // =======================================
            // ========================================================================
            // Parse OpenType Layout FeatureList
            //
            // Shared by GSUB and GPOS.
            // ========================================================================

            // ========================================================================
// Parse OpenType Layout LookupList
//
// Shared by GSUB and GPOS.
// For now this only validates/enumerates the Lookup structures.
// It does not interpret GSUB/GPOS subtables yet.
// ========================================================================

            inline bool parseLayoutLookups(OpenTypeByteStream lookupList) noexcept
            {
                uint16_t lookupCount = 0;

                if (!lookupList.readUInt16(lookupCount))
                    return false;

                // Offset16 lookupOffsets[lookupCount]
                if (lookupCount > lookupList.remaining() / 2)
                    return false;

                // Keep the offset array separate from the cursor used
                // to parse individual Lookup tables.
                auto lookupOffsets =
                    lookupList.subStream(2, size_t(lookupCount) * 2);

                if (!lookupOffsets.isValid())
                    return false;

                for (uint16_t i = 0; i < lookupCount; ++i)
                {
                    uint16_t lookupOffset = 0;

                    if (!lookupOffsets.readOffset16(lookupOffset))
                        return false;

                    if (lookupOffset == 0)
                        continue;

                    // lookupOffset is relative to LookupList.
                    auto lookup =
                        lookupList.subStream(lookupOffset);

                    if (!lookup.isValid())
                        continue;

                    uint16_t lookupType = 0;
                    uint16_t lookupFlag = 0;
                    uint16_t subTableCount = 0;

                    if (!lookup.readUInt16(lookupType))
                        continue;

                    if (!lookup.readUInt16(lookupFlag))
                        continue;

                    if (!lookup.readUInt16(subTableCount))
                        continue;

                    // Offset16 subTableOffsets[subTableCount]
                    if (subTableCount > lookup.remaining() / 2)
                        continue;

                    for (uint16_t j = 0; j < subTableCount; ++j)
                    {
                        uint16_t subTableOffset = 0;

                        if (!lookup.readOffset16(subTableOffset))
                            return false;

                        if (subTableOffset == 0)
                            continue;

                        // Subtable offsets are relative to the Lookup table.
                        auto subtable =
                            lookup.subStream(subTableOffset);

                        if (!subtable.isValid())
                            continue;

                        // Don't interpret the subtable yet.
                        //
                        // lookupType tells us what it means, and that meaning
                        // differs between GSUB and GPOS.
                    }

                    // lookupFlag bit 0x0010 means MarkFilteringSet follows
                    // the subtable offset array.
                    if (lookupFlag & 0x0010)
                    {
                        uint16_t markFilteringSet = 0;

                        if (!lookup.readUInt16(markFilteringSet))
                            continue;
                    }
                }

                return true;
            }

            inline bool parseLayoutFeatureTags(const TableRecord& table) noexcept
            {
                OpenTypeByteStream stream(table.data);

                // --------------------------------------------------------------------
                // GSUB / GPOS header
                //
                // uint16   majorVersion
                // uint16   minorVersion
                // Offset16 scriptListOffset
                // Offset16 featureListOffset
                // Offset16 lookupListOffset
                //
                // Version 1.1 additionally has:
                // Offset32 featureVariationsOffset
                // --------------------------------------------------------------------

                uint16_t majorVersion = 0;
                uint16_t minorVersion = 0;

                if (!stream.readUInt16(majorVersion))
                    return false;

                if (!stream.readUInt16(minorVersion))
                    return false;

                if (majorVersion != 1 || minorVersion > 1)
                    return false;

                uint16_t scriptListOffset = 0;
                uint16_t featureListOffset = 0;
                uint16_t lookupListOffset = 0;

                if (!stream.readOffset16(scriptListOffset))
                    return false;

                if (!stream.readOffset16(featureListOffset))
                    return false;

                if (!stream.readOffset16(lookupListOffset))
                    return false;

                uint32_t featureVariationsOffset = 0;

                if (minorVersion == 1)
                {
                    if (!stream.readOffset32(featureVariationsOffset))
                        return false;
                }

                // These offsets are relative to the beginning of the
                // GSUB/GPOS table.
                if (scriptListOffset == 0 ||
                    featureListOffset == 0 ||
                    lookupListOffset == 0)
                {
                    return false;
                }

                auto scriptList =
                    stream.subStream(scriptListOffset);

                auto featureList =
                    stream.subStream(featureListOffset);

                auto lookupList =
                    stream.subStream(lookupListOffset);

                if (!scriptList.isValid() ||
                    !featureList.isValid() ||
                    !lookupList.isValid())
                {
                    return false;
                }

                // Version 1.1 FeatureVariations is optional.
                // We aren't parsing it yet, but validate its offset when present.
                if (featureVariationsOffset != 0)
                {
                    auto featureVariations =
                        stream.subStream(featureVariationsOffset);

                    if (!featureVariations.isValid())
                        return false;
                }

                // --------------------------------------------------------------------
                // Validate/enumerate the LookupList separately.
                // --------------------------------------------------------------------

                if (!parseLayoutLookups(lookupList))
                    return false;

                // --------------------------------------------------------------------
                // FeatureList
                //
                // uint16        featureCount
                // FeatureRecord featureRecords[featureCount]
                //
                // FeatureRecord:
                //   Tag      featureTag
                //   Offset16 featureOffset
                //
                // For feature discovery we only need the tag.  The Feature table
                // referenced by featureOffset will become important when we connect
                // features to their lookup indices.
                // --------------------------------------------------------------------

                uint16_t featureCount = 0;

                if (!featureList.readUInt16(featureCount))
                    return false;

                // Each FeatureRecord is six bytes.
                if (featureCount > featureList.remaining() / 6)
                    return false;

                for (uint16_t i = 0; i < featureCount; ++i)
                {
                    Tag featureTag = 0;
                    uint16_t featureOffset = 0;

                    if (!featureList.readUInt32(featureTag))
                        return false;

                    if (!featureList.readOffset16(featureOffset))
                        return false;

                    // A FeatureRecord offset is relative to FeatureList.
                    // Validate it even though we aren't consuming the Feature table yet.
                    if (featureOffset == 0)
                        continue;

                    auto feature =
                        featureList.subStream(featureOffset);

                    if (!feature.isValid())
                        continue;

                    mFeatures.addFeature(featureTag);
                }

                return true;
            }


            inline void parseGSUBTable() noexcept
            {
                const TableRecord* table =
                    getTable(TagConstants::GSUB);

                if (!table)
                    return;

                mFeatureFlags |= FeatureFlags::GSUB;

                parseLayoutFeatureTags(*table);
            }

            // ========================================================================
            // Fleshed out parseGPOSTable()
            // ========================================================================

            inline void parseGPOSTable() noexcept
            {
                const TableRecord* table =
                    getTable(TagConstants::GPOS);

                if (!table)
                    return;

                mFeatureFlags |= FeatureFlags::GPOS;

                parseLayoutFeatureTags(*table);
            }

            // ========================================================================
            // Parse feature tags from 'kern' table (old-style kerning)
            // ========================================================================

            inline void parseKERNTable() noexcept {
                const TableRecord* table = getTable(TagConstants::KERN);
                if (!table) return;

                mFeatureFlags |= FeatureFlags::KERN;

                // Old-style kern table has subtable formats
                // We just add the 'kern' feature since kerning is present
                mFeatures.addFeature(OTAG("kern"));
            }

            // ========================================================================
            // Helper: Parse Horizontal Metrics
            // ========================================================================

            inline void parseHorizontalMetrics(uint16_t glyphId,
                const TableRecord& hmtxTable,
                const TableRecord& hheaTable,
                GlyphMetrics& metrics) const noexcept {
                const ByteSpan& hmtxData = hmtxTable.data;
                const ByteSpan& hheaData = hheaTable.data;

                if (hheaData.size() < 36) return;

                uint16_t numHMetrics = (static_cast<uint16_t>(hheaData[34]) << 8) |
                    static_cast<uint16_t>(hheaData[35]);

                if (glyphId < numHMetrics) {
                    size_t pos = glyphId * 4;
                    if (pos + 4 <= hmtxData.size()) {
                        metrics.advanceWidth = (static_cast<int16_t>(hmtxData[pos]) << 8) |
                            static_cast<int16_t>(hmtxData[pos + 1]);
                        metrics.leftSideBearing = (static_cast<int16_t>(hmtxData[pos + 2]) << 8) |
                            static_cast<int16_t>(hmtxData[pos + 3]);
                    }
                }
                else {
                    size_t pos = (numHMetrics - 1) * 4;
                    if (pos + 4 <= hmtxData.size()) {
                        metrics.advanceWidth = (static_cast<int16_t>(hmtxData[pos]) << 8) |
                            static_cast<int16_t>(hmtxData[pos + 1]);
                        size_t lsbPos = numHMetrics * 4 + (glyphId - numHMetrics) * 2;
                        if (lsbPos + 2 <= hmtxData.size()) {
                            metrics.leftSideBearing = (static_cast<int16_t>(hmtxData[lsbPos]) << 8) |
                                static_cast<int16_t>(hmtxData[lsbPos + 1]);
                        }
                    }
                }
            }

            // ========================================================================
            // Helper: Parse Bounding Box
            // ========================================================================

            inline void parseBoundingBox(uint16_t glyphId,
                const TableRecord& glyfTable,
                const TableRecord& locaTable,
                const TableRecord& headTable,
                GlyphMetrics& metrics) const noexcept {
                const ByteSpan& headData = headTable.data;
                if (headData.size() < 52) return;

                bool isLong = (static_cast<uint16_t>(headData[50]) << 8) |
                    static_cast<uint16_t>(headData[51]);
                isLong = isLong != 0;

                uint32_t location = 0;
                const ByteSpan& locaData = locaTable.data;

                if (isLong) {
                    if (glyphId * 4 + 4 <= locaData.size()) {
                        location = (static_cast<uint32_t>(locaData[glyphId * 4]) << 24) |
                            (static_cast<uint32_t>(locaData[glyphId * 4 + 1]) << 16) |
                            (static_cast<uint32_t>(locaData[glyphId * 4 + 2]) << 8) |
                            static_cast<uint32_t>(locaData[glyphId * 4 + 3]);
                    }
                }
                else {
                    if (glyphId * 2 + 2 <= locaData.size()) {
                        location = (static_cast<uint32_t>(locaData[glyphId * 2]) << 8) |
                            static_cast<uint32_t>(locaData[glyphId * 2 + 1]);
                        location *= 2;
                    }
                }

                const ByteSpan& glyfData = glyfTable.data;
                if (location + 10 <= glyfData.size()) {
                    metrics.xMin = (static_cast<int16_t>(glyfData[location + 2]) << 8) |
                        static_cast<int16_t>(glyfData[location + 3]);
                    metrics.yMin = (static_cast<int16_t>(glyfData[location + 4]) << 8) |
                        static_cast<int16_t>(glyfData[location + 5]);
                    metrics.xMax = (static_cast<int16_t>(glyfData[location + 6]) << 8) |
                        static_cast<int16_t>(glyfData[location + 7]);
                    metrics.yMax = (static_cast<int16_t>(glyfData[location + 8]) << 8) |
                        static_cast<int16_t>(glyfData[location + 9]);
                }
            }

            // ========================================================================
            // Helper: Glyph Index for CMAP Encoding
            // ========================================================================

// ========================================================================
// Helper: Glyph Index for CMAP Encoding
// ========================================================================

inline uint32_t glyphIndexForEncoding(
    uint32_t codepoint,
    const CmapEncoding& enc) const noexcept
{
    const TableRecord* cmapTable = getTable(TagConstants::CMAP);
    if (!cmapTable)
        return 0;

    OpenTypeByteStream cmap(cmapTable->data);

    // EncodingRecord.offset is relative to the beginning of cmap.
    // enc.length was validated when the cmap table was parsed.
    auto subtable = cmap.subStream(enc.offset, enc.length);
    if (!subtable.isValid())
        return 0;

    switch (enc.format)
    {
    case 0:
        return glyphIndexFormat0(codepoint, subtable);

    case 4:
        return glyphIndexFormat4(codepoint, subtable);

    case 12:
        return glyphIndexFormat12(codepoint, subtable);

    default:
        return 0;
    }
}


// ========================================================================
// cmap Format 0
// ========================================================================
//
// uint16  format
// uint16  length
// uint16  language
// uint8   glyphIdArray[256]
//
inline uint32_t glyphIndexFormat0(
    uint32_t codepoint,
    OpenTypeByteStream subtable) const noexcept
{
    if (codepoint > 0xFF)
        return 0;

    uint16_t format;
    uint16_t length;
    uint16_t language;

    if (!subtable.readUInt16(format))
        return 0;

    if (!subtable.readUInt16(length))
        return 0;

    if (!subtable.readUInt16(language))
        return 0;

    if (format != 0)
        return 0;

    // Header + 256-byte glyphIdArray.
    if (length < 262 || subtable.size() < 262)
        return 0;

    if (!subtable.seek(6 + codepoint))
        return 0;

    uint8_t glyphId;
    if (!subtable.readUInt8(glyphId))
        return 0;

    return glyphId;
}


// ========================================================================
// cmap Format 4
// ========================================================================
//
// uint16  format
// uint16  length
// uint16  language
// uint16  segCountX2
// uint16  searchRange
// uint16  entrySelector
// uint16  rangeShift
//
// uint16  endCode[segCount]
// uint16  reservedPad
// uint16  startCode[segCount]
// int16   idDelta[segCount]
// uint16  idRangeOffsets[segCount]
// uint16  glyphIdArray[]
//
inline uint32_t glyphIndexFormat4(
    uint32_t codepoint,
    OpenTypeByteStream subtable) const noexcept
{
    // Format 4 handles only 16-bit character codes.
    if (codepoint > 0xFFFF)
        return 0;

    uint16_t format;
    uint16_t length;
    uint16_t language;
    uint16_t segCountX2;
    uint16_t searchRange;
    uint16_t entrySelector;
    uint16_t rangeShift;

    if (!subtable.readUInt16(format))
        return 0;

    if (!subtable.readUInt16(length))
        return 0;

    if (!subtable.readUInt16(language))
        return 0;

    if (!subtable.readUInt16(segCountX2))
        return 0;

    if (!subtable.readUInt16(searchRange))
        return 0;

    if (!subtable.readUInt16(entrySelector))
        return 0;

    if (!subtable.readUInt16(rangeShift))
        return 0;

    if (format != 4)
        return 0;

    // segCountX2 must represent a non-zero number of 16-bit segments.
    if (segCountX2 == 0 || (segCountX2 & 1))
        return 0;

    const size_t segCount = segCountX2 / 2;

    // Minimum size:
    //
    // 14 byte header
    // endCode        2 * segCount
    // reservedPad    2
    // startCode      2 * segCount
    // idDelta        2 * segCount
    // idRangeOffsets 2 * segCount
    //
    // = 16 + 8 * segCount
    //
    if (subtable.size() < 16)
        return 0;

    if (segCount > (subtable.size() - 16) / 8)
        return 0;

    const size_t endCodeOffset = 14;
    const size_t startCodeOffset =
        endCodeOffset + segCount * 2 + 2;

    const size_t idDeltaOffset =
        startCodeOffset + segCount * 2;

    const size_t idRangeOffsetOffset =
        idDeltaOffset + segCount * 2;

    // Make bounded views over the four parallel arrays.
    auto endCodes =
        subtable.subStream(endCodeOffset, segCount * 2);

    auto startCodes =
        subtable.subStream(startCodeOffset, segCount * 2);

    auto idDeltas =
        subtable.subStream(idDeltaOffset, segCount * 2);

    auto idRangeOffsets =
        subtable.subStream(idRangeOffsetOffset, segCount * 2);

    if (!endCodes.isValid() ||
        !startCodes.isValid() ||
        !idDeltas.isValid() ||
        !idRangeOffsets.isValid())
        return 0;

    auto readUInt16At =
        [](const OpenTypeByteStream& array,
           size_t index,
           uint16_t& value) noexcept -> bool
    {
        auto item = array.subStream(index * 2, 2);
        return item.isValid() && item.readUInt16(value);
    };

    auto readInt16At =
        [](const OpenTypeByteStream& array,
           size_t index,
           int16_t& value) noexcept -> bool
    {
        auto item = array.subStream(index * 2, 2);
        return item.isValid() && item.readInt16(value);
    };

    // The endCode array is sorted in increasing order.
    // Find the first segment whose endCode >= codepoint.
    size_t left = 0;
    size_t right = segCount;

    while (left < right)
    {
        const size_t mid = left + (right - left) / 2;

        uint16_t endCode;
        if (!readUInt16At(endCodes, mid, endCode))
            return 0;

        if (codepoint > endCode)
            left = mid + 1;
        else
            right = mid;
    }

    if (left >= segCount)
        return 0;

    const size_t segment = left;

    uint16_t startCode;
    uint16_t endCode;
    int16_t idDelta;
    uint16_t idRangeOffset;

    if (!readUInt16At(startCodes, segment, startCode))
        return 0;

    if (!readUInt16At(endCodes, segment, endCode))
        return 0;

    if (!readInt16At(idDeltas, segment, idDelta))
        return 0;

    if (!readUInt16At(idRangeOffsets, segment, idRangeOffset))
        return 0;

    if (codepoint < startCode || codepoint > endCode)
        return 0;

    // Simple delta mapping.
    if (idRangeOffset == 0)
    {
        return static_cast<uint16_t>(
            codepoint + static_cast<int32_t>(idDelta));
    }

    // idRangeOffset is measured from the location of this
    // segment's idRangeOffset WORD -- not from glyphIdArray.
    //
    // glyph address =
    //
    //   &idRangeOffsets[segment]
    //       + idRangeOffset
    //       + 2 * (codepoint - startCode)
    //
    const size_t rangeWordOffset =
        idRangeOffsetOffset + segment * 2;

    const size_t glyphOffset =
        rangeWordOffset +
        idRangeOffset +
        static_cast<size_t>(codepoint - startCode) * 2;

    auto glyphEntry = subtable.subStream(glyphOffset, 2);
    if (!glyphEntry.isValid())
        return 0;

    uint16_t glyphId;
    if (!glyphEntry.readUInt16(glyphId))
        return 0;

    // Glyph ID zero means missing glyph. The idDelta is NOT
    // applied in that case.
    if (glyphId == 0)
        return 0;

    return static_cast<uint16_t>(
        glyphId + static_cast<int32_t>(idDelta));
}


// ========================================================================
// cmap Format 12
// ========================================================================
//
// uint16  format
// uint16  reserved
// uint32  length
// uint32  language
// uint32  numGroups
//
// SequentialMapGroup groups[numGroups]:
//     uint32 startCharCode
//     uint32 endCharCode
//     uint32 startGlyphID
//
inline uint32_t glyphIndexFormat12(
    uint32_t codepoint,
    OpenTypeByteStream subtable) const noexcept
{
    uint16_t format;
    uint16_t reserved;
    uint32_t length;
    uint32_t language;
    uint32_t numGroups;

    if (!subtable.readUInt16(format))
        return 0;

    if (!subtable.readUInt16(reserved))
        return 0;

    if (!subtable.readUInt32(length))
        return 0;

    if (!subtable.readUInt32(language))
        return 0;

    if (!subtable.readUInt32(numGroups))
        return 0;

    if (format != 12 || reserved != 0)
        return 0;

    // We are now immediately before the group array.
    // Validate multiplication without overflow.
    if (numGroups > subtable.remaining() / 12)
        return 0;

    auto groups = subtable.subStream(16);
    if (!groups.isValid())
        return 0;

    size_t left = 0;
    size_t right = static_cast<size_t>(numGroups);

    while (left < right)
    {
        const size_t mid = left + (right - left) / 2;

        auto group = groups.subStream(mid * 12, 12);
        if (!group.isValid())
            return 0;

        uint32_t startCharCode;
        uint32_t endCharCode;
        uint32_t startGlyphId;

        if (!group.readUInt32(startCharCode))
            return 0;

        if (!group.readUInt32(endCharCode))
            return 0;

        if (!group.readUInt32(startGlyphId))
            return 0;

        if (codepoint < startCharCode)
        {
            right = mid;
        }
        else if (codepoint > endCharCode)
        {
            left = mid + 1;
        }
        else
        {
            return startGlyphId +
                (codepoint - startCharCode);
        }
    }

    return 0;
}
        };

    } // namespace opentype
} // namespace waavs
*/