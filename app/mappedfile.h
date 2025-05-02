#pragma once

/*
	mmap is the rough equivalent of the mmap() function on Linux
	This basically allows you to memory map a file, which means you 
	can access a pointer to the file's contents without having to 
	go through IO routines.

	Usage:
	local m = MappedFile::create_shared(filename)

	local bs = binstream(m:getPointer(), #m)
*/
#include <SDKDDKVer.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <cstdio>
#include <string>
#include <cstdint>
#include <memory>


namespace waavs
{
    struct MappedFile
    {
        void* fData{};
        size_t fSize{};
        bool fIsValid{};

        HANDLE fFileHandle{};
        HANDLE fMapHandle{};

    public:
        MappedFile(HANDLE filehandle, HANDLE maphandle, void* data, size_t length) noexcept
            :fData(data)
            , fSize(length)
            , fFileHandle(filehandle)
            , fMapHandle(maphandle)
        {
            fIsValid = true;
        }


        MappedFile() noexcept
            : fData(nullptr)
            , fSize(0)
            , fIsValid(false)
            , fFileHandle(nullptr)
            , fMapHandle(nullptr)
        {}

        virtual ~MappedFile() noexcept { close(); }

        bool isValid() const noexcept { return fIsValid; }
        void* data() const noexcept { return fData; }
        size_t size() const noexcept { return fSize; }

        bool close() noexcept
        {
            if (fData != nullptr) {
                ::UnmapViewOfFile(fData);
                fData = nullptr;
            }

            if (fMapHandle != INVALID_HANDLE_VALUE) {
                ::CloseHandle(fMapHandle);
                fMapHandle = INVALID_HANDLE_VALUE;
            }

            if (fFileHandle != INVALID_HANDLE_VALUE) {
                ::CloseHandle(fFileHandle);
                fFileHandle = INVALID_HANDLE_VALUE;
            }

            return true;
        }



        // factory method
        // desiredAccess - GENERIC_READ, GENERIC_WRITE, GENERIC_EXECUTE
        // shareMode - FILE_SHARE_READ, FILE_SHARE_WRITE
        // creationDisposition - CREATE_ALWAYS, CREATE_NEW, OPEN_ALWAYS, OPEN_EXISTING, TRUNCATE_EXISTING
        static std::shared_ptr<MappedFile> create_shared(const std::string& filename,
            uint32_t desiredAccess = GENERIC_READ,
            uint32_t shareMode = FILE_SHARE_READ,
            uint32_t disposition = OPEN_EXISTING) noexcept
        {
            uint32_t flagsAndAttributes = (FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS);

            const char* fname = filename.c_str();
            HANDLE filehandle = ::CreateFileA(fname,
                desiredAccess,
                shareMode,
                nullptr,
                disposition,
                flagsAndAttributes,
                nullptr);

            //print("mmap.create() -    File Handle: ", filehandle)

            if (filehandle == INVALID_HANDLE_VALUE) {
                // BUGBUG - do anything more than returning invalid?
                printf("Could not create/open file for mmap: %d  %s", static_cast<int>(::GetLastError()), fname);
                return {};
            }

            // BUGBUG
            // Need to check whether we're opening for writing or not
            // if we're opening for writing, then we don't want to 
            // limit the size in CreateFileMappingA
            LARGE_INTEGER psize;
            ::GetFileSizeEx(filehandle, &psize);
            int64_t size = psize.QuadPart;

            // Open mapping
            HANDLE maphandle = ::CreateFileMappingA(filehandle, nullptr, PAGE_READONLY, psize.HighPart, psize.LowPart, nullptr);
            //printf("CREATE File Mapping: ", maphandle)

            if ((maphandle == INVALID_HANDLE_VALUE) || (maphandle == nullptr))
            {
                //error("Could not create file map: "..tostring(ffi.C.GetLastError()))
                // close file handle and set it to invalid
                ::CloseHandle(filehandle);
                filehandle = INVALID_HANDLE_VALUE;

                return {};
            }

            // Open view
            void* data = ::MapViewOfFile(maphandle, FILE_MAP_READ, 0, 0, 0);
            //print("MAP VIEW: ", m.map)

            if (data == nullptr) {
                ::CloseHandle(maphandle);
                ::CloseHandle(filehandle);
                maphandle = INVALID_HANDLE_VALUE;
                filehandle = INVALID_HANDLE_VALUE;

                return {};
            }

            return std::make_shared<MappedFile>(filehandle, maphandle, data, size);
        }
    };
}