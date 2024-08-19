#pragma once

#include <filesystem>

#include "aligned_alloc.hpp"

namespace fs = std::filesystem;

class Data {
   public:
    alignedVector<float> vertex;
    alignedVector<uint16_t> index;

    void load(const fs::path& path);

    size_t maxBufferSize();
};
