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
            notice("Restaure une copie valide avant d'exporter.", true);
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
            notice("Impossible d'enregistrer la sauvegarde.", true);
            return;
        }
        FLAlertLayer::create("Sauvegarde creee",
            fmt::format("Ta sauvegarde a ete enregistree ici :\n{}", path.string()).c_str(), "OK")->show();
        notice("Sauvegarde creee sur cet appareil.");
    }

    bool knownId(int id) const {
        return std::find(m_levelIds.begin(), m_levelIds.end(), id) != m_levelIds.end();
    }

    void restore() {
        const auto path = Mod::get()->getSaveDir() / "training-backup.json";
        std::error_code error;
        const auto size = std::filesystem::file_size(path, error);
        if (error || size > 2000000) {
            notice("Sauvegarde absente ou trop volumineuse.", true);
            return;
        }
        std::ifstream stream(path, std::ios::binary);
        if (!stream) {
            notice("Impossible d'ouvrir la sauvegarde.", true);
            return;
        }
        const std::string raw((std::istreambuf_iterator<char>(stream)), {});
        auto json = matjson::parse(raw);
        if (!json || json.unwrap()["schema"].asInt().unwrapOr(0) != 1) {
            notice("Format de sauvegarde non pris en charge.", true);
            return;
        }
        training::Store candidate;
        if (!candidate.decode(json.unwrap()["payload"].asString().unwrapOr("")) ||
            (candidate.goal && !knownId(candidate.goal))) {
            notice("Sauvegarde invalide. Donnees conservees.", true);
            return;
        }
        for (auto const& [id, value] : candidate.levels) {
            if (!knownId(id)) {
                notice("La sauvegarde contient un niveau hors du parcours.", true);
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
            notice("Copie de secours impossible. Import annule.", true);
            return;
        }
        for (auto const& [id, value] : current.levels)
            candidate.levels[id].best = std::max(candidate.levels[id].best, value.best);
        current = std::move(candidate);
        current.save();
        notice("Sauvegarde restauree. Anciennes donnees conservees.");
    }

    void onRestore(CCObject*) {
        Ref<ToolsPopup> self(this);
        createQuickPopup("Restaurer la sauvegarde ?",
            "Remplacer tes notes, runs, favoris et startpos par la sauvegarde locale ? "
            "Tes donnees actuelles seront conservees dans une copie de secours. Tes records ne diminueront pas.",
            "Annuler", "Restaurer", [self](FLAlertLayer*, bool confirmed) {
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
        label(m_mainLayer, "OUTILS", 158, 225, 220, .46f);
        button(m_buttonMenu, this, menu_selector(ToolsPopup::onClose),
               "X", 299, 225, 22, 22);
        label(m_mainLayer, "Tes donnees restent sur cet appareil.",
              158, 191, 284, .28f, muted);
        button(m_buttonMenu, this, menu_selector(ToolsPopup::onBackup),
               "EXPORTER LA SAUVEGARDE", 158, 157, 264, 28, true);
        button(m_buttonMenu, this, menu_selector(ToolsPopup::onRestore),
               "RESTAURER LA SAUVEGARDE", 158, 119, 264, 28);
        button(m_buttonMenu, this, menu_selector(ToolsPopup::onSettings),
               "PARAMETRES DU MOD", 158, 81, 264, 28);
        m_notice = label(m_mainLayer, "Fichier : training-backup.json dans le dossier du mod.",
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
