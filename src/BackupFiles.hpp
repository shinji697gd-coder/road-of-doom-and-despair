#pragma once

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace training {
inline std::vector<std::filesystem::path> backupFiles(std::filesystem::path const& directory) {
    std::vector<std::filesystem::path> result;
    std::error_code error;
    std::filesystem::directory_iterator iterator(directory, error), end;
    for (; !error && iterator != end; iterator.increment(error)) {
        const auto name = iterator->path().filename().string();
        if (name.starts_with("auto-") && name.ends_with(".json") && name.size() > 10 &&
            std::all_of(name.begin() + 5, name.end() - 5,
                        [](unsigned char c) { return std::isdigit(c); }) &&
            iterator->is_regular_file(error))
            result.push_back(iterator->path());
    }
    std::sort(result.rbegin(), result.rend());
    return result;
}

inline bool writeSnapshot(std::filesystem::path const& directory,
                          std::string const& stamp, std::string const& bytes) {
    if (stamp.empty() || !std::all_of(stamp.begin(), stamp.end(),
        [](unsigned char c) { return std::isdigit(c); })) return false;
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (error) return false;
    const auto target = directory / ("auto-" + stamp + ".json");
    if (std::filesystem::exists(target, error) || error) return false;
    const auto temporary = directory / ("auto-" + stamp + ".tmp");
    std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
    stream << bytes;
    stream.close();
    if (stream.fail()) return false;
    std::filesystem::rename(temporary, target, error);
    if (error) return false;
    const auto files = backupFiles(directory);
    for (std::size_t i = 10; i < files.size(); ++i) {
        std::filesystem::remove(files[i], error);
        if (error) return false;
    }
    return true;
}
}
