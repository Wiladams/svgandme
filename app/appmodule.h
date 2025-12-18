#pragma once

#include "Windows.h"
#include "bspan.h"

namespace waavs {

	// This is NOT thread safe
    // an internal static buffer is used to hold the filename
	static ByteSpan getModuleFileName()
	{
		static char lpFilename[512];

		static DWORD nSize = 512;
		static DWORD actualSize = 0;
		static HMODULE hModule = NULL;
		static bool called = false;

		if (!called)
		{
			// Ideally, we'd call this a few times with increasing buffer size
			// when it's not enough, the return value is: ERROR_INSUFFICIENT_BUFFER
			// we would then increase the size, and try again.
			// This is not as nice as the interfaces that tell you directly how much 
			// space is needed to be allocated.
			// for now, we'll go with a large buffer size, and leave it at that
			actualSize = ::GetModuleFileNameA(hModule, lpFilename, nSize);
			called = true;
		}

        ByteSpan result;
		result.resetFromSize(lpFilename, actualSize);
		return result;
	}

	static ByteSpan getModuleCommandLine()
	{
		// BUGBUG, should make a copy of command line, since
		// we don't manage the lifetime
		// We are assuming the lifetime of the returned char *
		// is the duration of the program
		static char* lpCmdLine = ::GetCommandLineA();
		static bool called = false;

		ByteSpan result;
        result.resetFromSize(lpCmdLine, strlen(lpCmdLine));

		return result;
	}
	
	// Command line argument iterator
	// Given a command line argument list
	// iterate the command arguments, and return them
	// one by one
	struct ArgIter {
		ByteSpan fArgList{};

		ArgIter(ByteSpan argList) : fArgList(argList) {}

		ByteSpan currentArgument()
		{

		}

		ByteSpan currentValue()
		{

		}
	};
}