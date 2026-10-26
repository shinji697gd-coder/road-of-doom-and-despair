#pragma once

#include <Geode/modify/PlayLayer.hpp>
#include <Geode/binding/PlayerObject.hpp>
#include "Persistence.hpp"

// Start only after PlayLayer is ready: dontCreateObjects is a loader option,
// not a reason to discard a real session. The timer observes gameplay without
// relying on a physics/postUpdate path that another mod may replace.
class $modify(RoadmapSessionStats, PlayLayer) {
    struct Fields {
        training::Session session;
        training::AttemptCounter counter;
        std::chrono::steady_clock::time_point tick = std::chrono::steady_clock::now();
        std::chrono::steady_clock::time_point saved = tick;
        bool initialized = false;
        bool tracking = false;
        bool stopped = false;
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
    void ensureTracking() {
        if (!m_fields->initialized || m_fields->tracking || m_fields->stopped ||
            !m_level || !m_player1 || m_useReplay || m_isPlatformer ||
            m_level->m_levelType == GJLevelType::Editor ||
            !geode::Mod::get()->getSettingValue<bool>("auto-session-stats")) return;
        auto& data = training::Extras::get();
        const int played = static_cast<int>(m_level->m_levelID);
        const int owner = data.ownerOf(played);
        if (!owner || !data.writable || data.nextSession >= 1000000000000LL) return;
        auto& session = m_fields->session;
        session.id = ++data.nextSession;
        session.owner = owner;
        session.played = played;
        session.started = session.ended = training::now();
        m_fields->tracking = true;
        m_fields->tick = std::chrono::steady_clock::now();
        readBest();
        flush();
        if (geode::Mod::get()->getSettingValue<bool>("diagnostic-logging"))
            geode::log::info("[RDoD][Stats] started {} owner={} played={} practice={} test={}",
                training::levelName(owner), owner, played, m_isPracticeMode, m_isTestMode);
    }
    void sample(float) {
        ensureTracking();
        const auto clock = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration<double>(clock - m_fields->tick).count();
        m_fields->tick = clock;
        if (!m_fields->tracking) return;
        const bool active = m_player1 && m_started && !m_player1->m_isDead &&
            !m_isPaused && !m_hasCompletedLevel && !m_useReplay &&
            geode::Mod::get()->getSettingValue<bool>("auto-session-stats");
        const auto attempts = m_fields->session.attempts;
        m_fields->counter.tick(m_fields->session,
            m_isPracticeMode || m_isTestMode || m_startPosObject, elapsed, active);
        if ((attempts[0] + attempts[1] == 0 &&
             m_fields->session.attempts[0] + m_fields->session.attempts[1] != 0) ||
            clock - m_fields->saved >= std::chrono::seconds(30)) flush();
    }
    bool init(GJGameLevel* level, bool replay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, replay, dontCreateObjects)) return false;
        m_fields->initialized = true;
        // Register on the real Cocos lifecycle; initializing a loading layer
        // does not by itself start a counted attempt.
        schedule(schedule_selector(RoadmapSessionStats::sample), .1f);
        return true;
    }
    void onEnterTransitionDidFinish() {
        PlayLayer::onEnterTransitionDidFinish();
        sample(0);
        if (!m_fields->tracking &&
            geode::Mod::get()->getSettingValue<bool>("diagnostic-logging"))
            geode::log::info("[RDoD][Stats] not started: id={} ready={} replay={} platformer={} enabled={} writable={}",
                m_level ? static_cast<int>(m_level->m_levelID) : 0, m_player1 != nullptr,
                m_useReplay, m_isPlatformer,
                geode::Mod::get()->getSettingValue<bool>("auto-session-stats"), training::Extras::get().writable);
    }
    void startGame() {
        PlayLayer::startGame();
        sample(0);
    }
    void resetLevel() {
        sample(0);
        readBest();
        m_fields->counter.suspend();
        PlayLayer::resetLevel();
        m_fields->counter.resume();
        m_fields->tick = std::chrono::steady_clock::now();
        sample(0);
    }
    void destroyPlayer(PlayerObject* player, GameObject* object) {
        sample(0);
        PlayLayer::destroyPlayer(player, object);
        if (m_player1 && m_player1->m_isDead) {
            m_fields->counter.close();
            readBest();
        }
    }
    void levelComplete() {
        sample(0);
        PlayLayer::levelComplete();
        m_fields->counter.close();
        readBest();
        flush();
    }
    void togglePracticeMode(bool practice) {
        sample(0);
        m_fields->counter.suspend();
        PlayLayer::togglePracticeMode(practice);
        m_fields->counter.resume();
        m_fields->tick = std::chrono::steady_clock::now();
        sample(0);
    }
    void pauseGame(bool unfocused) {
        sample(0);
        PlayLayer::pauseGame(unfocused);
        flush();
    }
    void resume() {
        PlayLayer::resume();
        m_fields->tick = std::chrono::steady_clock::now();
    }
    void stopTracking() {
        sample(0);
        readBest();
        flush();
        m_fields->tracking = false;
        m_fields->stopped = true;
        unschedule(schedule_selector(RoadmapSessionStats::sample));
    }
    void onQuit() {
        stopTracking();
        PlayLayer::onQuit();
    }
    void onExit() {
        stopTracking();
        PlayLayer::onExit();
    }
};
