#pragma once

#include <Geode/ui/GeodeUI.hpp>
#include <fstream>
#include <functional>
#include "../Training.hpp"
#include "Widgets.hpp"

namespace roadui {
class ToolsPopup final : public Popup {
    std::vector<int> m_levelIds;
    std::function<void()> m_changed;
    CCLabelBMFont* m_notice = nullptr;

    void notice(std::string const& text, bool error = false) {
        m_notice->setString(text.c_str());
        m_notice->limitLabelWidth(280, .27f, .14f);
        m_notice->setColor(error ? ccColor3B{240, 170, 140} : muted);
    }

    void onBackup(CCObject*) {
        if (!training::Store::get().writable) {
            notice("Restore a valid save before making a backup.", true);
            return;
        }
        const auto path = Mod::get()->getSaveDir() / "training-backup.json";
        matjson::Value backup;
        backup["schema"] = 1;
        backup["payload"] = training::Store::get().encode();
        std::ofstream stream(path, std::ios::binary);
        stream << backup.dump();
        stream.close();
        if (stream.fail()) {
            notice("Backup could not be written.", true);
            return;
        }
        FLAlertLayer::create("Backup saved",
            fmt::format("Your training backup was saved to:\n{}", path.string()).c_str(), "OK")->show();
        notice("Backup saved on this device.");
    }

    bool knownId(int id) const {
        return std::find(m_levelIds.begin(), m_levelIds.end(), id) != m_levelIds.end();
    }

    void restore() {
        const auto path = Mod::get()->getSaveDir() / "training-backup.json";
        std::error_code error;
        const auto size = std::filesystem::file_size(path, error);
        if (error || size > 2000000) {
            notice("Backup is missing or too large.", true);
            return;
        }
        std::ifstream stream(path, std::ios::binary);
        if (!stream) {
            notice("Backup could not be opened.", true);
            return;
        }
        const std::string raw((std::istreambuf_iterator<char>(stream)), {});
        auto json = matjson::parse(raw);
        if (!json || json.unwrap()["schema"].asInt().unwrapOr(0) != 1) {
            notice("Backup has an unsupported format.", true);
            return;
        }
        training::Store candidate;
        if (!candidate.decode(json.unwrap()["payload"].asString().unwrapOr("")) ||
            (candidate.goal && !knownId(candidate.goal))) {
            notice("Invalid backup. Your data was kept.", true);
            return;
        }
        for (auto const& [id, value] : candidate.levels) {
            if (!knownId(id)) {
                notice("Backup contains a level outside this roadmap.", true);
                return;
            }
        }
        auto& current = training::Store::get();
        // Preserve the raw saved value as well if it was unreadable.
        const auto previous = current.writable ? current.encode() :
            Mod::get()->getSavedValue<std::string>("training-v1", "");
        std::ofstream before(Mod::get()->getSaveDir() / "training-before-import.txt", std::ios::binary);
        before << previous;
        before.close();
        if (before.fail()) {
            notice("Could not preserve the current save. Import cancelled.", true);
            return;
        }
        for (auto const& [id, value] : current.levels)
            candidate.levels[id].best = std::max(candidate.levels[id].best, value.best);
        current = std::move(candidate);
        current.save();
        notice("Backup restored. Previous data was kept in a recovery file.");
    }

    void onRestore(CCObject*) {
        Ref<ToolsPopup> self(this);
        createQuickPopup("Restore training backup?",
            "Replace notes, runs, favorites and startpos with your local backup? "
            "Your current data will be kept in a recovery file. Normal bests will not decrease.",
            "Cancel", "Restore", [self](FLAlertLayer*, bool confirmed) {
                if (confirmed) self->restore();
            });
    }

    void onSettings(CCObject*) { openSettingsPopup(Mod::get()); }

    void onClose(CCObject* sender) override {
        if (m_changed) m_changed();
        Popup::onClose(sender);
    }

    bool setup(std::vector<int> ids, std::function<void()> changed) {
        if (!Popup::init(316, 242)) return false;
        m_levelIds = std::move(ids);
        m_changed = std::move(changed);
        m_bgSprite->setVisible(false);
        m_closeBtn->setVisible(false);
        m_buttonMenu->setPosition({0, 0});
        m_buttonMenu->setAnchorPoint({0, 0});
        auto frame = panel(m_mainLayer, 0, 0, 316, 242);
        frame->setZOrder(-1);
        rect(frame, 1, 208, 314, 33, inset);
        label(m_mainLayer, "TOOLS", 158, 225, 220, .46f);
        button(m_buttonMenu, this, menu_selector(ToolsPopup::onClose),
               "X", 299, 225, 22, 22);
        label(m_mainLayer, "Training data stays on this device.",
              158, 191, 284, .28f, muted);
        button(m_buttonMenu, this, menu_selector(ToolsPopup::onBackup),
               "EXPORT BACKUP", 158, 157, 264, 28, true);
        button(m_buttonMenu, this, menu_selector(ToolsPopup::onRestore),
               "RESTORE BACKUP", 158, 119, 264, 28);
        button(m_buttonMenu, this, menu_selector(ToolsPopup::onSettings),
               "MOD SETTINGS", 158, 81, 264, 28);
        m_notice = label(m_mainLayer, "Backups use training-backup.json in the mod save folder.",
                         158, 34, 280, .25f, muted);
        fitPopup(m_mainLayer, 316, 242);
        return true;
    }

public:
    static ToolsPopup* create(std::vector<int> ids, std::function<void()> changed = {}) {
        auto popup = new ToolsPopup();
        if (popup->setup(std::move(ids), std::move(changed))) {
            popup->autorelease();
            return popup;
        }
        delete popup;
        return nullptr;
    }
};
}
