#pragma once

#include "bstream.h"
#include "mappedfile.h"

#include <memory>

// An object that given a filename will return a BStream
// It will retain the file handle until the object is destroyed
struct FileStreamer
{
	std::shared_ptr<waavs::MappedFile> fFile;
	waavs::BStream fStream;

	FileStreamer(std::shared_ptr<waavs::MappedFile> afile)
		: fFile(afile)
	{
		// create fStream from afile
		fStream = waavs::BStream(fFile->data(), fFile->size());
	}

	const waavs::ByteSpan & span() const
	{
		return fStream.span();
	}
	
	waavs::BStream& stream() { return fStream; }

	bool close() const { return fFile->close(); }
	
	static std::shared_ptr<FileStreamer> createFromFilename(const std::string& filename)
	{
		auto afile = waavs::MappedFile::create_shared(filename);
		if (!afile)
		{
			printf("Failed to open file %s\n", filename.c_str());
			return nullptr;
		}

		auto fstream = std::make_shared<FileStreamer>(afile);

		return fstream;
	}
};
