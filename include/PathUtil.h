#ifndef _PATH_UTIL_H_
#define _PATH_UTIL_H_

#include <filesystem>
#include <string>

// Resolves a path against a base directory and ensures it is not a directory traversal attack.
std::filesystem::path resolveSecurePath(const std::filesystem::path& baseDir, const std::filesystem::path& filePath);

#endif // _PATH_UTIL_H_
