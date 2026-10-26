#include "../src/BackupFiles.hpp"
#include <cassert>
#include <chrono>

int main() {
    const auto directory = std::filesystem::temp_directory_path() /
        ("rdod-backup-tests-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(directory);
    // Files outside the managed filename pattern are never pruned.
    { std::ofstream keep(directory / "personal.json"); keep << "keep"; }
    assert(!training::writeSnapshot(directory, "../invalid", "{}"));
    for (int i = 100; i < 112; ++i)
        assert(training::writeSnapshot(directory, std::to_string(i), "{}"));
    auto files = training::backupFiles(directory);
    assert(files.size() == 10 && files.front().filename() == "auto-111.json");
    assert(!std::filesystem::exists(directory / "auto-100.json"));
    assert(std::filesystem::exists(directory / "personal.json"));
    assert(!training::writeSnapshot(directory, "111", "overwrite"));
    // Cleanup only the exact temporary test files, never a recursive directory.
    for (auto const& file : files) std::filesystem::remove(file);
    std::filesystem::remove(directory / "personal.json");
    std::filesystem::remove(directory);
}
