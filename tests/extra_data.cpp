#include "../src/ExtraData.hpp"
#include <cassert>

int main() {
    constexpr int forest = 11402965, acu = 61079355, copy = 91440952;
    training::Extras data;
    assert(data.setCopy(forest, copy));
    assert(!data.setCopy(acu, copy)); // A copy cannot count for two originals.
    assert(!data.setCopy(forest, acu));
    assert(data.ownerOf(copy) == forest && data.ownerOf(acu) == acu);
    assert(!data.ownerOf(123));
    assert(data.observeBest(forest, 68, 1700000000));
    assert(data.levels[forest].records.empty()); // No fabricated historical date.
    assert(!data.observeBest(forest, 60, 1700000001));
    assert(data.observeBest(forest, 70, 1700000002));
    assert(data.levels[forest].records.size() == 1);
    auto& target = data.levels[forest];
    target.goalKind = 2; target.goalFrom = 40; target.goalTo = 100;
    data.observeRun(forest, 41, 100);
    assert(!target.goalDone);
    data.observeRun(forest, 39, 100);
    assert(target.goalDone);

    training::Session session;
    session.id = ++data.nextSession;
    session.owner = forest; session.played = copy;
    session.started = session.ended = 1700000000;
    training::AttemptCounter counter;
    counter.tick(session, false, .1, true);
    counter.tick(session, false, .1, true);
    counter.tick(session, false, 10, false); // Pause/death excluded.
    assert(session.attempts[0] == 1 && session.seconds[0] == .2);
    counter.close(); counter.close(); // Dual death/reset must not double count.
    counter.tick(session, true, .1, true);
    counter.tick(session, true, 10, true); // Long suspension not counted as play time.
    assert(session.attempts[1] == 1 && session.seconds[1] == .1);
    data.putSession(session);
    data.putSession(session);
    assert(data.sessions.size() == 1);
    training::Extras restored;
    assert(restored.decode(data.encode()) && restored.encode() == data.encode());
    const auto before = restored.encode();
    assert(!restored.decode("RDX 2 0 0 0"));
    assert(!restored.decode("RDX 1 0 1 0\n123 0 -1 0 0 100 0 0"));
    assert(!restored.decode("RDX 1 0 1 0\n11402965 0 -1 2 80 50 0 0"));
    assert(!restored.decode("RDX 1 0 0 0 junk"));
    assert(restored.encode() == before);
}
