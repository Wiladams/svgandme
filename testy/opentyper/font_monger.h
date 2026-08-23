
// opentype_fontmonger.h - Font collection manager

#pragma once

#include "font_interfaces.h"
#include "font_faceset.h"
#include "opentype_container.h"

#include <vector>
#include <filesystem>
#include <fstream>
#include <string>


namespace fs = std::filesystem;


namespace waavs
{

    struct FontMonger : public FontFaceSet
    {
    private:
        //std::vector<FontFace> fFaces;

    public:

        // ====================================================================
        // IProvideFontFaces
        // ====================================================================

        //size_t fontFaceCount() const noexcept override
        //{
        //    return fFaces.size();
        //}


        //FontFace fontFace(size_t index) const noexcept override
        //{
        //    if (index >= fFaces.size())
        //        return FontFace{};

        //    return fFaces[index];
        //}


        // ====================================================================
        // Add already-created faces/providers
        // ====================================================================

        //void addFontFace(const FontFace& face)
        //{
        //    if (!face.isValid())
        //        return;

        //    fFaces.push_back(face);
        //}

        //void addFontFaces(const IProvideFontFaces& provider)
        //{
        //    const size_t count =
        //        provider.fontFaceCount();

        //    for (size_t i = 0; i < count; ++i)
        //    {
        //        FontFace face =
        //            provider.fontFace(i);

        //        if (face.isValid())
        //            fFaces.push_back(face);
        //    }
        //}


        // ====================================================================
        // Add a font file
        //
        // A file may contain one face:
        //
        //     TTF
        //     OTF
        //
        // or many faces:
        //
        //     TTC
        //
        // FontMonger doesn't need to care. The container exposes
        // IProvideFontFaces and we simply absorb all valid faces.
        // ====================================================================

        bool addFile(const fs::path& path)
        {
            SharedMemBuff buffer;

            if (!readFile(path, buffer))
                return false;

            opentype::OpenTypeContainer container(buffer);

            if (!container.isValid())
                return false;

            addFontFaces(container);

            return true;
        }


        bool addFile(const char* filepath)
        {
            if (!filepath || !*filepath)
                return false;

            return addFile(fs::path(filepath));
        }


        // ====================================================================
        // Add every supported font file in a directory.
        //
        // recursive == true:
        //
        //     walk all descendant directories.
        //
        // recursive == false:
        //
        //     inspect only the supplied directory.
        //
        // The return value is the number of font faces added, not the
        // number of files. This distinction matters for TTC files.
        // ====================================================================

        bool addDirectory( const fs::path& directory, bool recursive = true)
        {
            std::error_code ec;

            if (!fs::exists(directory, ec) ||
                !fs::is_directory(directory, ec))
            {
                return false;
            }

            //const size_t before = fFaces.size();

            if (recursive)
            {
                fs::recursive_directory_iterator iter(
                    directory,
                    fs::directory_options::skip_permission_denied,
                    ec);

                fs::recursive_directory_iterator end;

                while (!ec && iter != end)
                {
                    const fs::directory_entry& entry =
                        *iter;

                    if (entry.is_regular_file(ec))
                    {
                        const fs::path& path =
                            entry.path();

                        if (isSupportedFontFile(path))
                            addFile(path);
                    }

                    iter.increment(ec);
                }
            }
            else
            {
                fs::directory_iterator iter(
                    directory,
                    fs::directory_options::skip_permission_denied,
                    ec);

                fs::directory_iterator end;

                while (!ec && iter != end)
                {
                    const fs::directory_entry& entry = *iter;

                    if (entry.is_regular_file(ec))
                    {
                        const fs::path& path = entry.path();

                        if (isSupportedFontFile(path))
                            addFile(path);
                    }

                    iter.increment(ec);
                }
            }

            return true;
        }


        bool addDirectory( const char* directory, bool recursive = true)
        {
            if (!directory || !*directory)
                return false;

            return addDirectory( fs::path(directory), recursive);
        }


        // ====================================================================
        // Utility
        // ====================================================================

        //void clear()
        //{
        //    fFaces.clear();
        //}


        //bool empty() const noexcept
        //{
        //    return fFaces.empty();
        //}


        //size_t size() const noexcept
        //{
        //    return fFaces.size();
        //}


    private:

        // ====================================================================
        // Read a file into SharedMemBuff.
        //
        // Eventually this could be replaced by mmap/file mapping without
        // changing addFile() or addDirectory().
        // ====================================================================

        static bool readFile(const fs::path& path, SharedMemBuff& buffer)
        {
            std::ifstream file(
                path,
                std::ios::binary |
                std::ios::ate);

            if (!file.is_open())
                return false;

            const std::streamsize size =
                file.tellg();

            if (size <= 0)
                return false;

            file.seekg(
                0,
                std::ios::beg);

            if (!buffer.resetFromSize(
                static_cast<size_t>(size)))
            {
                return false;
            }

            if (!file.read(
                reinterpret_cast<char*>(
                    buffer.data()),
                size))
            {
                buffer.reset();
                return false;
            }

            return true;
        }


        // ====================================================================
        // File-name level filtering.
        //
        // This is intentionally only a cheap first-pass test.
        //
        // OpenTypeContainer remains responsible for determining whether the
        // actual bytes represent a valid supported font container.
        //
        // When WOFF / WOFF2 support arrives, this list simply grows.
        // ====================================================================

        static bool isSupportedFontFile(const fs::path& path)
        {
            std::string ext = path.extension().string();

            for (char& ch : ext)
            {
                if (ch >= 'A' && ch <= 'Z')
                    ch =
                    static_cast<char>(
                        ch - 'A' + 'a');
            }

            return
                ext == ".ttf" ||
                ext == ".otf" ||
                ext == ".ttc";

            // Eventually:
            //
            //     || ext == ".woff"
            //     || ext == ".woff2";
        }
    };

}
