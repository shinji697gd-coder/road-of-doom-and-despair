#pragma once

#include <Geode/modify/PlayLayer.hpp>
#include <Geode/binding/PlayerObject.hpp>
#include "Persistence.hpp"

// Passive observation only. No physics, progress, completion or native attempt
// fields are modified. An original and its chosen copy keep separate source IDs.
class $modify(RoadmapSessionStats, PlayLayer) {
    struct Fields {
        training::Session session;
        training::AttemptCounter counter;
        std::chrono::steady_clock::time_point tick = std::chrono::steady_clock::now();
        std::chrono::steady_clock::time_point saved = tick;
        bool tracking = false;
    };

    void readBest() {
        if (m_fields->tracking && m_level && m_fields->session.played == m_fields->session.owner)
            training::observeBest(m_fields->session.owner, m_level->getNormalPercent());
    }
    void flush() {
        if (!m_fields->tracking) return;
        auto& data = training::Extras::get();
        if (!data.writable) return;
        m_fields->session.ended = std::max(m_fields->session.started, training::now());
        data.putSession(m_fields->session);
        data.save();
        m_fields->saved = std::chrono::steady_clock::now();
    }

    bool init(GJGameLevel* level, bool replay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, replay, dontCreateObjects)) return false;
        auto& data = training::Extras::get();
        const int played = static_cast<int>(level->m_levelID);
        const int owner = data.ownerOf(played);
        if (!replay && !dontCreateObjects && owner && data.writable &&
            level->m_levelType != GJLevelType::Editor && !m_isPlatformer &&
            geode::Mod::get()->getSettingValue<bool>("auto-session-stats") &&
            data.nextSession < 1000000000000LL) {
            auto& session = m_fields->session;
            session.id = ++data.nextSession;
            session.owner = owner;
            session.played = played;
            session.started = session.ended = training::now();
            m_fields->tracking = true;
            readBest();
            flush();
        }
        return true;
    }
    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        if (!m_fields->tracking) return;
        const auto clock = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration<double>(clock - m_fields->tick).count();
        m_fields->tick = clock;
        const bool active = m_player1 && !m_isPaused && isGameplayActive() &&
            geode::Mod::get()->getSettingValue<bool>("auto-session-stats");
        const auto attempts = m_fields->session.attempts;
        m_fields->counter.tick(m_fields->session,
            m_isPracticeMode || m_isTestMode || m_startPosObject, elapsed, active);
        if ((attempts[0] + attempts[1] == 0 &&
             m_fields->session.attempts[0] + m_fields->session.attempts[1] != 0) ||
            clock - m_fields->saved >= std::chrono::seconds(30)) flush();
    }
    void resetLevel() {
        readBest();
        m_fields->counter.close();
        PlayLayer::resetLevel();
        m_fields->tick = std::chrono::steady_clock::now();
    }
    void destroyPlayer(PlayerObject* player, GameObject* object) {
        PlayLayer::destroyPlayer(player, object);
        if (m_player1 && m_player1->m_isDead) {
            m_fields->counter.close();
            readBest();
        }
    }
    void levelComplete() {
        PlayLayer::levelComplete();
        m_fields->counter.close();
        readBest();
        flush();
    }
    void togglePracticeMode(bool practice) {
        m_fields->counter.close();
        PlayLayer::togglePracticeMode(practice);
        m_fields->tick = std::chrono::steady_clock::now();
    }
    void pauseGame(bool unfocused) {
        PlayLayer::pauseGame(unfocused);
        flush();
    }
    void resume() {
        PlayLayer::resume();
        m_fields->tick = std::chrono::steady_clock::now();
    }
    void onQuit() {
        readBest();
        flush();
        m_fields->tracking = false;
        PlayLayer::onQuit();
    }
    void onExit() {
        readBest();
        flush();
        m_fields->tracking = false;
        PlayLayer::onExit();
    }
};
