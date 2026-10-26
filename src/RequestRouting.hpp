#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <utility>

namespace roadnet {
// The native delegate slot is borrowed only for a synchronous call. A delegate
// that another mod installs during that call must not be overwritten on return.
template <class Delegate>
class DelegateScope {
    Delegate*& m_slot;
    Delegate* m_previous;
    Delegate* m_selected;
public:
    DelegateScope(Delegate*& slot, Delegate* selected)
      : m_slot(slot), m_previous(slot), m_selected(selected) { m_slot = selected; }
    ~DelegateScope() { if (m_slot == m_selected) m_slot = m_previous; }
    DelegateScope(DelegateScope const&) = delete;
    DelegateScope& operator=(DelegateScope const&) = delete;
};

// Keep canceled replies until native completion: they still belong to this
// mod and must not be delivered to whichever GD menu happens to be open later.
template <class Reply>
class PendingReplies {
public:
    using Callback = std::function<void(Reply const&)>;
private:
    struct Pending { std::shared_ptr<Reply> reply; Callback callback; };
    std::map<std::string, Pending> m_pending;
public:
    std::shared_ptr<Reply> find(std::string const& key) const {
        auto found = m_pending.find(key);
        return found == m_pending.end() ? nullptr : found->second.reply;
    }
    void attach(std::string const& key, std::shared_ptr<Reply> reply, Callback callback) {
        m_pending[key] = {std::move(reply), std::move(callback)};
    }
    void cancel(std::string const& key) {
        if (auto found = m_pending.find(key); found != m_pending.end())
            found->second.callback = {};
    }
    void complete(std::string const& key, std::shared_ptr<Reply> const& reply) {
        auto found = m_pending.find(key);
        if (found == m_pending.end() || found->second.reply != reply) return;
        auto callback = std::move(found->second.callback);
        m_pending.erase(found);
        if (callback) callback(*reply);
    }
};
}
