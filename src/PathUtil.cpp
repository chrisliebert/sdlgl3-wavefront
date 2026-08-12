#include "PathUtil.h"
#include <filesystem>
#include <iostream>

std::filesystem::path resolveSecurePath(const std::filesystem::path& baseDir, const std::filesystem::path& filePath) {
    if (filePath.is_absolute()) {
        std::cerr << "Error: Absolute paths are not allowed: " << filePath << std::endl;
        return {};
    }

    std::filesystem::path combinedPath = baseDir / filePath;
    std::filesystem::path canonicalPath = std::filesystem::weakly_canonical(combinedPath);
    std::filesystem::path canonicalBase = std::filesystem::weakly_canonical(baseDir);

    auto baseIter = canonicalBase.begin();
    auto pathIter = canonicalPath.begin();

    while (baseIter != canonicalBase.end() && pathIter != canonicalPath.end()) {
        if (*baseIter != *pathIter) {
            return {};
        }
        ++baseIter;
        ++pathIter;
    }

    if (baseIter != canonicalBase.end()) {
        return {};
    }

    return canonicalPath;
}
