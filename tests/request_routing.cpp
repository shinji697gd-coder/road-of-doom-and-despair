#include "../src/RequestRouting.hpp"
#include <cassert>

int main() {
    int native = 0, roadmap = 1, anotherMod = 2;
    int* slot = &native;
    {
        roadnet::DelegateScope<int> scope(slot, &roadmap);
        assert(slot == &roadmap);
    }
    assert(slot == &native); // A non-null, idle owner is not a busy request.
    {
        roadnet::DelegateScope<int> scope(slot, &roadmap);
        slot = &anotherMod;
    }
    assert(slot == &anotherMod);

    roadnet::PendingReplies<int> pending;
    int called = 0;
    auto page1 = std::make_shared<int>(1);
    auto page2 = std::make_shared<int>(2);
    pending.attach("page1", page1, [&](int) { ++called; });
    pending.attach("page2", page2, [&](int result) { called += result; });
    pending.cancel("page1"); // Closing/changing view must not revive its callback.
    assert(pending.find("page1") == page1);
    pending.complete("page2", page2); // Out-of-order delivery is key-scoped.
    assert(called == 2 && !pending.find("page2"));
    pending.complete("page1", page1);
    assert(called == 2 && !pending.find("page1"));

    auto retry = std::make_shared<int>(3);
    pending.attach("page1", page1, [&](int) { assert(false); });
    pending.attach("page1", retry, [&](int result) {
        called += result;
        // Completion may start another fetch for the same key.
        pending.attach("page1", page1, [&](int) { ++called; });
    });
    pending.complete("page1", page1); // A stale response cannot finish a retry.
    assert(pending.find("page1") == retry && called == 2);
    pending.complete("page1", retry);
    assert(pending.find("page1") == page1 && called == 5);
    pending.complete("page1", page1);
    assert(called == 6 && !pending.find("page1"));
}
