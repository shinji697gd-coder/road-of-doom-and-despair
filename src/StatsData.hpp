#pragma once

#include "ExtraData.hpp"

namespace training {
struct StatsRow {
    std::int64_t time = 0;
    std::string name;
    bool copy = false;
    int best = 0;
    std::array<int, 2> attempts{};
    std::array<double, 2> seconds{};
};
struct StatsView {
    std::vector<StatsRow> rows;
    std::array<std::int64_t, 2> attempts{};
    std::array<double, 2> seconds{};
};
// Project only by the saved owner/source IDs, never by today's preferred-copy
// mapping: changing a preference must not move or hide old sessions.
inline StatsView statsView(Extras const& data, int owner, int tab) {
    StatsView view;
    if (tab == 2) {
        for (auto const& [id, level] : data.levels) {
            if (owner && id != owner) continue;
            for (auto const& record : level.records) {
                StatsRow row;
                row.time = record.observed;
                row.name = levelName(id);
                row.best = record.best;
                view.rows.push_back(std::move(row));
            }
        }
    } else {
        for (auto const& session : data.sessions) {
            const bool copy = session.played != session.owner;
            if ((owner && session.owner != owner) || copy != (tab == 1) ||
                session.attempts[0] + session.attempts[1] == 0) continue;
            StatsRow row;
            row.time = session.started;
            row.name = levelName(session.owner);
            row.copy = copy;
            row.attempts = session.attempts;
            row.seconds = session.seconds;
            for (int i = 0; i < 2; ++i) {
                view.attempts[i] += session.attempts[i];
                view.seconds[i] += session.seconds[i];
            }
            view.rows.push_back(std::move(row));
        }
    }
    std::stable_sort(view.rows.begin(), view.rows.end(),
        [](StatsRow const& a, StatsRow const& b) { return a.time > b.time; });
    return view;
}
}
