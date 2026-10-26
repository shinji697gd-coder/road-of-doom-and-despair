#pragma once

#include <Geode/Geode.hpp>
#include <chrono>
#include <ctime>
#include "TrainingData.hpp"
#include "ExtraData.hpp"
#include "BackupFiles.hpp"

namespace training {
using namespace geode::prelude;
inline std::int64_t now() { return static_cast<std::int64_t>(std::time(nullptr)); }
inline std::filesystem::path autoBackupDir() { return Mod::get()->getSaveDir() / "auto-backups"; }

inline void autoBackup() {
    if (!Mod::get()->getSettingValue<bool>("auto-backups")) return;
    static auto last = std::chrono::steady_clock::time_point{};
    const auto clock = std::chrono::steady_clock::now();
    if (last != std::chrono::steady_clock::time_point{} &&
        clock - last < std::chrono::minutes(10)) return;
    const auto raw = Mod::get()->getSavedValue<std::string>("training-v1", "");
    const auto extra = Mod::get()->getSavedValue<std::string>("training-extra-v1", "");
    if (raw.empty() && extra.empty()) return;
    Store base;
    Extras additions;
    if ((!raw.empty() && !base.decode(raw)) || (!extra.empty() && !additions.decode(extra))) return;
    matjson::Value envelope;
    envelope["schema"] = 2;
    envelope["payload"] = raw.empty() ? base.encode() : raw;
    envelope["extras"] = extra.empty() ? additions.encode() : extra;
    const auto stamp = std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
    if (writeSnapshot(autoBackupDir(), stamp, envelope.dump())) last = clock;
    else log::warn("[RDoD] Automatic backup failed; previous snapshots retained");
}
inline void persistToDisk() {
    // setSavedValue alone is normally flushed when GD closes. Session
    // checkpoints need an explicit write on the main game thread.
    if (auto result = Mod::get()->saveData(); !result)
        log::warn("[RDoD] Unable to persist mod data: {}", result.unwrapErr());
}
inline void Store::save() {
    if (!writable) return;
    autoBackup();
    Mod::get()->setSavedValue("training-v1", encode());
    persistToDisk();
}
inline Store& Store::get() {
    static Store store;
    static bool loaded = false;
    if (!loaded) {
        loaded = true;
        const auto raw = Mod::get()->getSavedValue<std::string>("training-v1", "");
        if (!raw.empty() && !store.decode(raw)) {
            store.writable = false;
            log::error("[RDoD] Invalid training save: writing disabled, original retained");
        }
    }
    return store;
}
inline void Extras::save() {
    if (!writable) return;
    autoBackup();
    Mod::get()->setSavedValue("training-extra-v1", encode());
    persistToDisk();
}
inline Extras& Extras::get() {
    static Extras data;
    static bool loaded = false;
    if (!loaded) {
        loaded = true;
        const auto raw = Mod::get()->getSavedValue<std::string>("training-extra-v1", "");
        if (!raw.empty() && !data.decode(raw)) {
            data.writable = false;
            log::error("[RDoD] Invalid extra save: writing disabled, original retained");
        }
    }
    return data;
}
inline void observeBest(int id, int best) {
    if (!Mod::get()->getSettingValue<bool>("auto-sync-progress")) return;
    auto& data = Extras::get();
    if (data.writable && data.observeBest(id, std::clamp(best, 0, 100), now())) data.save();
}
inline std::string date(std::int64_t value) {
    const std::time_t timestamp = static_cast<std::time_t>(value);
    const auto local = std::localtime(&timestamp);
    char buffer[32]{};
    if (local) std::strftime(buffer, sizeof(buffer), "%d/%m/%Y %H:%M", local);
    return buffer;
}
inline std::string duration(double seconds) {
    const auto value = static_cast<std::int64_t>(seconds);
    if (value < 3600) return fmt::format("{}m {:02}s", value / 60, value % 60);
    return fmt::format("{}h {:02}m", value / 3600, value / 60 % 60);
}
inline std::string goalCaption(int owner) {
    const auto& goal = Extras::get().levels[owner];
    if (!goal.goalKind) return "Aucun objectif precis";
    return (goal.goalDone ? std::string("Atteint : ") : std::string("Objectif : ")) +
        (goal.goalKind == 1 ? fmt::format("{}% en normal", goal.goalTo) :
         fmt::format("{} - {}%", goal.goalFrom, goal.goalTo));
}
inline matjson::Value backupEnvelope() {
    matjson::Value result;
    result["schema"] = 2;
    result["payload"] = Store::get().writable ? Store::get().encode() :
        Mod::get()->getSavedValue<std::string>("training-v1", "");
    result["extras"] = Extras::get().writable ? Extras::get().encode() :
        Mod::get()->getSavedValue<std::string>("training-extra-v1", "");
    return result;
}
}
