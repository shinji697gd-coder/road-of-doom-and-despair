#pragma once

#include <Geode/Geode.hpp>
#include <Geode/binding/GameLevelManager.hpp>
#include <Geode/binding/GJSearchObject.hpp>
#include <Geode/modify/GameLevelManager.hpp>
#include "RequestRouting.hpp"

namespace roadnet {
using namespace geode::prelude;

struct LevelReply final : LevelManagerDelegate {
    std::string key;
    Ref<CCArray> levels;
    bool ready = false;
    bool success = false;

    explicit LevelReply(std::string value) : key(std::move(value)) {}

    void loadLevelsFinished(CCArray* value, char const* responseKey) override {
        if (responseKey && key != responseKey) return;
        levels = value;
        success = ready = true;
    }
    void loadLevelsFinished(CCArray* value, char const* key, int) override {
        loadLevelsFinished(value, key);
    }
    void loadLevelsFailed(char const* responseKey) override {
        if (responseKey && key != responseKey) return;
        ready = true;
        success = false;
    }
    void loadLevelsFailed(char const* key, int) override { loadLevelsFailed(key); }
};

inline PendingReplies<LevelReply> replies;

// False means the same key is already being fetched by a native menu. The
// caller waits for its cache instead of taking that menu's callback.
inline bool fetch(GJSearchObject* search, PendingReplies<LevelReply>::Callback callback) {
    auto manager = GameLevelManager::get();
    const std::string key = search->getKey();
    if (manager->isDLActive(key.c_str())) {
        if (auto reply = replies.find(key)) {
            replies.attach(key, reply, std::move(callback));
            return true;
        }
        return false;
    }
    auto reply = std::make_shared<LevelReply>(key);
    replies.attach(key, reply, std::move(callback));
    if (Mod::get()->getSettingValue<bool>("diagnostic-logging"))
        log::info("[RDoD][Fetch] start key={}", key);
    {
        DelegateScope<LevelManagerDelegate> scope(manager->m_levelManagerDelegate, reply.get());
        manager->getOnlineLevels(search);
    }
    // Native cache hits and some errors can complete synchronously.
    if (reply->ready) replies.complete(key, reply);
    return true;
}
}

// Route only RDoD-owned response keys. The shared delegate remains available
// to GD/other mods during the network wait, and is restored before UI changes.
class $modify(RoadmapLevelResponses, GameLevelManager) {
    void onGetOnlineLevelsCompleted(gd::string response, gd::string tag) {
        const std::string key = tag;
        auto reply = roadnet::replies.find(key);
        if (!reply) {
            GameLevelManager::onGetOnlineLevelsCompleted(response, tag);
            return;
        }
        {
            roadnet::DelegateScope<LevelManagerDelegate> scope(m_levelManagerDelegate, reply.get());
            GameLevelManager::onGetOnlineLevelsCompleted(response, tag);
        }
        if (geode::Mod::get()->getSettingValue<bool>("diagnostic-logging"))
            geode::log::info("[RDoD][Fetch] reply key={} ready={} success={}",
                            key, reply->ready, reply->success);
        if (reply->ready) roadnet::replies.complete(key, reply);
    }
};
