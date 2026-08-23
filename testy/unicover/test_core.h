#pragma once

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <vector>

static bool readFileData(const char* filename, std::vector<uint8_t>& data)
{
    std::ifstream input(filename, std::ios::binary | std::ios::ate);

    if (!input)
        return false;

    const std::streamsize size = input.tellg();

    if (size <= 0)
        return false;

    input.seekg(0, std::ios::beg);

    data.resize(static_cast<size_t>(size));

    return static_cast<bool>(
        input.read(reinterpret_cast<char*>(data.data()), size));
}
