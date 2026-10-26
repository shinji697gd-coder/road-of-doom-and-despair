#include "../src/StatsData.hpp"
#include <cassert>

int main() {
    constexpr int forest = 11402965, fearNot = 100817977, crazyII = 47620786;
    assert(training::levelName(fearNot) == "Fear Not");
    assert(training::levelName(crazyII) == "CraZy II");
    assert(!training::knownLevel(123));
    training::Extras data;
    data.levels[fearNot].records.push_back({5, 1700000020});
    data.levels[crazyII].records.push_back({3, 1700000010});
    auto records = training::statsView(data, 0, 2);
    assert(records.rows.size() == 2 && records.rows[0].name == "Fear Not");
    assert(records.rows[1].name == "CraZy II" && records.rows[1].best == 3);
    training::Session original, copy, other, empty;
    original.owner = forest; original.played = forest;
    original.attempts = {4, 2}; original.seconds = {10, 5}; original.started = 1700000000;
    copy.owner = forest; copy.played = 91440952;
    copy.attempts = {0, 7}; copy.seconds = {0, 23}; copy.started = 1700000001;
    other.owner = fearNot; other.played = fearNot;
    other.attempts = {3, 0}; other.started = 1700000002;
    empty.owner = forest; empty.played = forest;
    data.sessions = {original, copy, other, empty};
    auto originals = training::statsView(data, 0, 0);
    auto copies = training::statsView(data, 0, 1);
    assert(originals.rows.size() == 2 && originals.attempts[0] == 7 && originals.attempts[1] == 2);
    assert(copies.rows.size() == 1 && copies.rows[0].name == "Forest Temple");
    assert(copies.attempts[1] == 7 && copies.seconds[1] == 23);
    auto one = training::statsView(data, forest, 0);
    assert(one.rows.size() == 1 && one.attempts[0] == 4);
    assert(data.setCopy(forest, 95644789)); // Old sessions retain their source after a copy change.
    assert(training::statsView(data, forest, 1).attempts[1] == 7);
    assert(training::statsView(data, 123, 0).rows.empty());
}
