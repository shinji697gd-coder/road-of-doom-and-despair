#pragma once

#include "level_data.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace training {
inline std::map<int, std::string> const& levelCatalog() {
    static const auto catalog = [] {
        std::map<int, std::string> result;
        std::istringstream input(road::levels);
        std::string line;
        while (std::getline(input, line)) {
            const auto separator = line.find('|');
            if (separator != std::string::npos)
                result.emplace(std::stoi(line.substr(separator + 1)), line.substr(0, separator));
        }
        return result;
    }();
    return catalog;
}
inline bool knownLevel(int id) { return levelCatalog().contains(id); }
inline std::string levelName(int id) {
    auto found = levelCatalog().find(id);
    return found == levelCatalog().end() ? "Niveau hors parcours" : found->second;
}

struct Record { int best = 0; std::int64_t observed = 0; };
struct ExtraLevel {
    int copy = 0;
    int seenBest = -1;
    int goalKind = 0; // 0 none, 1 normal percentage, 2 manually recorded run
    double goalFrom = 0, goalTo = 100;
    bool goalDone = false;
    std::vector<Record> records;
};
struct Session {
    std::int64_t id = 0, started = 0, ended = 0;
    int owner = 0, played = 0;
    std::array<int, 2> attempts{}; // normal / practice or test, including copied levels
    std::array<double, 2> seconds{};
};

// First active frame starts an attempt, not init/reset callbacks. Death,
// dual-player callbacks and pause/resume cannot increment it twice.
struct AttemptCounter {
    bool open = false;
    int suspended = 0;
    void close() { open = false; }
    void suspend() { ++suspended; close(); }
    void resume() { if (suspended > 0) --suspended; }
    void tick(Session& session, bool practice, double elapsed, bool active) {
        if (suspended || !active || !std::isfinite(elapsed) || elapsed < 0) return;
        const auto mode = practice ? 1 : 0;
        if (!open) {
            session.attempts[mode] = std::min(10000000, session.attempts[mode] + 1);
            open = true;
        }
        if (elapsed <= 1) session.seconds[mode] = std::min(31536000., session.seconds[mode] + elapsed);
    }
};

struct Extras {
    std::map<int, ExtraLevel> levels;
    std::vector<Session> sessions;
    std::int64_t nextSession = 0;
    bool writable = true;

    int ownerOf(int played) const {
        if (knownLevel(played)) return played;
        for (auto const& [owner, level] : levels)
            if (level.copy == played && played > 0) return owner;
        return 0;
    }
    bool setCopy(int owner, int copy) {
        if (!knownLevel(owner) || copy < 0 || (copy && knownLevel(copy))) return false;
        for (auto const& [id, level] : levels)
            if (copy && id != owner && level.copy == copy) return false;
        levels[owner].copy = copy;
        return true;
    }
    bool observeBest(int owner, int best, std::int64_t now) {
        if (!knownLevel(owner) || best < 0 || best > 100 || now <= 0) return false;
        auto& level = levels[owner];
        bool changed = false;
        if (level.seenBest < 0) { level.seenBest = best; changed = true; }
        else if (best > level.seenBest) {
            level.seenBest = best;
            level.records.push_back({best, std::max(now,
                level.records.empty() ? std::int64_t{0} : level.records.back().observed)});
            if (level.records.size() > 100) level.records.erase(level.records.begin());
            changed = true;
        }
        if (level.goalKind == 1 && best >= level.goalTo && !level.goalDone) {
            level.goalDone = true;
            changed = true;
        }
        return changed;
    }
    void observeRun(int owner, double from, double to) {
        auto& level = levels[owner];
        if (level.goalKind == 2 && from <= level.goalFrom && to >= level.goalTo)
            level.goalDone = true;
    }
    void putSession(Session const& session) {
        auto found = std::find_if(sessions.begin(), sessions.end(),
            [&](Session const& value) { return value.id == session.id; });
        if (found != sessions.end()) *found = session;
        else sessions.push_back(session);
        if (sessions.size() > 500) sessions.erase(sessions.begin());
    }
    std::string encode() const {
        std::ostringstream out;
        out << std::setprecision(17) << "RDX 1 " << nextSession << ' '
            << levels.size() << ' ' << sessions.size() << '\n';
        for (auto const& [id, value] : levels) {
            out << id << ' ' << value.copy << ' ' << value.seenBest << ' '
                << value.goalKind << ' ' << value.goalFrom << ' ' << value.goalTo << ' '
                << value.goalDone << ' ' << value.records.size() << '\n';
            for (auto const& record : value.records)
                out << record.best << ' ' << record.observed << '\n';
        }
        for (auto const& value : sessions)
            out << value.id << ' ' << value.owner << ' ' << value.played << ' '
                << value.started << ' ' << value.ended << ' '
                << value.attempts[0] << ' ' << value.attempts[1] << ' '
                << value.seconds[0] << ' ' << value.seconds[1] << '\n';
        return out.str();
    }
    bool decode(std::string const& raw) {
        if (raw.size() > 2000000) return false;
        Extras candidate;
        std::istringstream input(raw);
        std::string magic;
        int schema, count, sessionCount;
        if (!(input >> magic >> schema >> candidate.nextSession >> count >> sessionCount) ||
            magic != "RDX" || schema != 1 || candidate.nextSession < 0 ||
            candidate.nextSession > 1000000000000LL || count < 0 || count > 75 ||
            sessionCount < 0 || sessionCount > 500) return false;
        std::set<int> copies;
        for (int i = 0; i < count; ++i) {
            int id, records;
            ExtraLevel value;
            if (!(input >> id >> value.copy >> value.seenBest >> value.goalKind >>
                  value.goalFrom >> value.goalTo >> value.goalDone >> records) ||
                !knownLevel(id) || candidate.levels.contains(id) || value.copy < 0 ||
                (value.copy && (knownLevel(value.copy) || !copies.insert(value.copy).second)) ||
                value.seenBest < -1 || value.seenBest > 100 || value.goalKind < 0 ||
                value.goalKind > 2 || !std::isfinite(value.goalFrom) ||
                !std::isfinite(value.goalTo) || value.goalFrom < 0 ||
                value.goalFrom >= value.goalTo || value.goalTo > 100 ||
                (value.goalKind == 1 && value.goalFrom != 0) ||
                records < 0 || records > 100) return false;
            int lastBest = 0;
            std::int64_t lastDate = 0;
            for (int j = 0; j < records; ++j) {
                Record record;
                if (!(input >> record.best >> record.observed) || record.best <= lastBest ||
                    record.best > value.seenBest || record.observed < lastDate ||
                    record.observed <= 0 || record.observed > 4102444800LL) return false;
                value.records.push_back(record);
                lastBest = record.best; lastDate = record.observed;
            }
            candidate.levels[id] = std::move(value);
        }
        std::set<std::int64_t> sessionIds;
        for (int i = 0; i < sessionCount; ++i) {
            Session value;
            if (!(input >> value.id >> value.owner >> value.played >> value.started >>
                  value.ended >> value.attempts[0] >> value.attempts[1] >>
                  value.seconds[0] >> value.seconds[1]) ||
                !knownLevel(value.owner) || value.played <= 0 ||
                (knownLevel(value.played) && value.played != value.owner) ||
                value.id <= 0 || value.id > candidate.nextSession ||
                !sessionIds.insert(value.id).second || value.started <= 0 ||
                value.ended < value.started || value.ended > 4102444800LL) return false;
            for (int mode = 0; mode < 2; ++mode)
                if (value.attempts[mode] < 0 || value.attempts[mode] > 10000000 ||
                    !std::isfinite(value.seconds[mode]) || value.seconds[mode] < 0 ||
                    value.seconds[mode] > 31536000) return false;
            candidate.sessions.push_back(value);
        }
        input >> std::ws;
        if (!input.eof()) return false;
        *this = std::move(candidate);
        return true;
    }
    void save();
    static Extras& get();
};
}
