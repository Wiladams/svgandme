// font_directory_view.h
//
// Lazy, forward-only directory source of FontFace values.
//
// Contract:
//
//     bool operator()(FontFace& face);
//
// Usage:
//
//     FontDirectoryView fonts("C:/Windows/Fonts");
//
//     FontFace face;
//     while (fonts(face))
//     {
//         ...
//     }
//
// The view is single-pass. Once exhausted, construct another view
// to traverse the directory again.

#pragma once

#include "font_face.h"
#include "opentype_container.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <utility>


namespace waavs
{
    namespace fs = std::filesystem;


    // ================================================================
    // Create an interned, absolute source-location string.
    // ================================================================

    inline FontName createNormalizedPath(const fs::path& path)
    {
        std::error_code ec;

        fs::path absolutePath = fs::absolute(path, ec);

        const fs::path& result = ec ? path : absolutePath;

        const std::string text = result.string();

        return WSNameSet::INTERN(text.c_str());
    }


    inline FontName createNormalizedPath(const char* path)
    {
        if (!path || !*path)
            return nullptr;

        return createNormalizedPath(
            fs::path(path));
    }


    // ================================================================
    // FontDirectoryView
    //
    // Lazily traverses a directory and produces FontFace values.
    //
    // Each supported file is opened as an OpenTypeContainer.
    // OpenTypeContainer is itself a FontFace generator, so this class
    // simply chains directory traversal to container traversal.
    //
    //     directory
    //         |
    //     font file
    //         |
    //     OpenTypeContainer
    //         |
    //     FontFace
    //
    // No FontFace values are retained by FontDirectoryView.
    // ================================================================

    class FontDirectoryView
    {
    private:
        fs::path fDirectory;
        bool fRecursive{ true };


        // ============================================================
        // Directory traversal state
        // ============================================================

        std::error_code fError;

        fs::directory_iterator fDirectoryIter;
        fs::directory_iterator fDirectoryEnd;

        fs::recursive_directory_iterator fRecursiveIter;
        fs::recursive_directory_iterator fRecursiveEnd;

        bool fStarted{ false };
        bool fFinished{ false };


        // ============================================================
        // Current font-resource generator
        //
        // A TTF/OTF normally yields one face.
        // A TTC may yield several.
        //
        // FontDirectoryView does not need to know which.
        // ============================================================

        std::unique_ptr<opentype::OpenTypeContainer>
            fCurrentContainer;


    public:
        FontDirectoryView() = default;


        explicit FontDirectoryView(
            fs::path directory,
            bool recursive = true)
            : fDirectory(std::move(directory))
            , fRecursive(recursive)
        {
        }


        // Stateful generator: movable, not copyable.
        FontDirectoryView(FontDirectoryView&&) = default;
        FontDirectoryView& operator=(FontDirectoryView&&) = default;


        // Don't allow copy constructor
        FontDirectoryView(const FontDirectoryView&) = delete;
        FontDirectoryView& operator=(const FontDirectoryView&) = delete;


        // ============================================================
        // FontFace generator contract
        //
        // true:
        //     face contains the next valid FontFace.
        //
        // false:
        //     the directory source is exhausted.
        // ============================================================

        bool operator()(FontFace& face)
        {
            face = {};

            if (fFinished)
                return false;


            if (!fStarted)
            {
                if (!beginTraversal())
                {
                    fFinished = true;
                    return false;
                }

                fStarted = true;
            }


            for (;;)
            {
                // ----------------------------------------------------
                // First let the current OpenType resource produce
                // another face.
                // ----------------------------------------------------

                if (fCurrentContainer)
                {
                    if ((*fCurrentContainer)(face))
                        return true;


                    // Current TTF / OTF / TTC is exhausted.
                    fCurrentContainer.reset();
                }


                // ----------------------------------------------------
                // Find another candidate font file.
                // ----------------------------------------------------

                fs::path path;

                if (!nextFontFile(path))
                {
                    fFinished = true;
                    return false;
                }


                // ----------------------------------------------------
                // Open the next file.
                //
                // Invalid or unreadable resources are skipped.
                // ----------------------------------------------------

                if (!openFontFile(path))
                    continue;


                // Loop back around.
                //
                // The newly created OpenTypeContainer will be asked
                // for its first face at the top of the loop.
            }
        }


        // ============================================================
        // State / information
        // ============================================================

        bool exhausted() const noexcept
        {
            return fFinished;
        }


        const fs::path& directory() const noexcept
        {
            return fDirectory;
        }

        bool recursive() const noexcept
        {
            return fRecursive;
        }

    private:

        // ============================================================
        // Begin filesystem traversal.
        // ============================================================

        bool beginTraversal()
        {
            fError.clear();


            if (!fs::exists(fDirectory, fError))
            {
                return false;
            }

            if (fError)
                return false;


            if (!fs::is_directory(fDirectory,fError))
            {
                return false;
            }

            if (fError)
                return false;


            if (fRecursive)
            {
                fRecursiveIter =
                    fs::recursive_directory_iterator(
                        fDirectory,
                        fs::directory_options::
                        skip_permission_denied,
                        fError);
            }
            else
            {
                fDirectoryIter =
                    fs::directory_iterator(
                        fDirectory,
                        fs::directory_options::
                        skip_permission_denied,
                        fError);
            }


            return !fError;
        }


        // ============================================================
        // Find the next candidate font file.
        // ============================================================

        bool nextFontFile(
            fs::path& path)
        {
            if (fRecursive)
            {
                return nextFontFileRecursive(
                    path);
            }

            return nextFontFileFlat(
                path);
        }


        bool nextFontFileFlat(fs::path& path)
        {
            while (fDirectoryIter !=
                fDirectoryEnd)
            {
                fs::directory_entry entry =
                    *fDirectoryIter;


                advanceFlatIterator();


                std::error_code ec;

                if (!entry.is_regular_file(ec))
                    continue;

                if (ec)
                    continue;


                const fs::path candidate =
                    entry.path();


                if (!isSupportedFontFile(
                    candidate))
                {
                    continue;
                }


                path = candidate;
                return true;
            }


            return false;
        }


        bool nextFontFileRecursive(fs::path& path)
        {
            while (fRecursiveIter !=
                fRecursiveEnd)
            {
                fs::directory_entry entry =
                    *fRecursiveIter;


                advanceRecursiveIterator();


                std::error_code ec;

                if (!entry.is_regular_file(ec))
                    continue;

                if (ec)
                    continue;


                const fs::path candidate =
                    entry.path();


                if (!isSupportedFontFile(
                    candidate))
                {
                    continue;
                }


                path = candidate;
                return true;
            }


            return false;
        }


        // ============================================================
        // Advance filesystem iterator.
        //
        // Permission or entry errors are tolerated.
        // ============================================================

        void advanceFlatIterator()
        {
            fError.clear();

            fDirectoryIter.increment(
                fError);

            if (fError)
                fError.clear();
        }


        void advanceRecursiveIterator()
        {
            fError.clear();

            fRecursiveIter.increment(
                fError);

            if (fError)
                fError.clear();
        }


        // ============================================================
        // Open one font resource.
        //
        // OpenTypeContainer is itself a FontFace generator:
        //
        //     bool operator()(FontFace&)
        //
        // The directory view therefore does not need to know whether
        // the file contains one face or many.
        // ============================================================

        bool openFontFile(const fs::path& path)
        {
            SharedMemBuff buffer;

            if (!readFile(
                path,
                buffer))
            {
                return false;
            }


            const FontName sourceLocation =
                createNormalizedPath(path);


            auto container =
                std::make_unique<
                opentype::OpenTypeContainer>(
                    buffer,
                    sourceLocation);


            if (!container->isValid())
                return false;


            fCurrentContainer =
                std::move(container);


            return true;
        }


        // ============================================================
        // Read complete font resource.
        //
        // OpenTypeFaceData ultimately retains the SharedMemBuff, so a
        // FontFace returned from the container remains valid after this
        // directory view advances to another file.
        // ============================================================

        static bool readFile( const fs::path& path, SharedMemBuff& buffer)
        {
            std::ifstream file(
                path,
                std::ios::binary |
                std::ios::ate);


            if (!file.is_open())
                return false;


            const std::streamsize size = file.tellg();


            if (size <= 0)
                return false;


            file.seekg(0, std::ios::beg);


            if (!buffer.resetFromSize(
                static_cast<size_t>(
                    size)))
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


        // ============================================================
        // Cheap filename-level filtering.
        //
        // OpenTypeContainer remains responsible for validating the
        // actual contents.
        // ============================================================

        static bool isSupportedFontFile(
            const fs::path& path)
        {
            std::string ext =
                path.extension().string();


            for (char& ch : ext)
            {
                if (ch >= 'A' &&
                    ch <= 'Z')
                {
                    ch =
                        static_cast<char>(
                            ch - 'A' + 'a');
                }
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

} // namespace waavs