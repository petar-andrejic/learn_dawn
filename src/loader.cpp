#include "loader.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

#include <fmt/format.h>

void Data::load(const fs::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error(fmt::format("Failed to open file {}", path.c_str()));
    }

    vertex.clear();
    index.clear();

    enum class Section { None, Points, Indices };

    Section currentSection = Section::None;

    float val;
    uint16_t idx;
    std::string line;

    while (!file.eof()) {
        getline(file, line);
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        if (line == "[points]") {
            currentSection = Section::Points;
        } else if (line == "[indices]") {
            currentSection = Section::Indices;
        } else if (line[0] == '#' || line.empty()) {
            continue;
        } else if (currentSection == Section::Points) {
            std::istringstream iss(line);
            for (int i = 0; i < 5; ++i) {
                iss >> val;
                vertex.push_back(val);
            }
        } else if (currentSection == Section::Indices) {
            std::istringstream iss(line);
            for (int i = 0; i < 3; ++i) {
                iss >> idx;
                index.push_back(idx);
            }
        }
    }
};

size_t Data::maxBufferSize() {
    size_t vertexSize = vertex.size() * sizeof(float);
    size_t indexSize = index.size() * sizeof(uint16_t);
    return std::max(vertexSize, indexSize);
};