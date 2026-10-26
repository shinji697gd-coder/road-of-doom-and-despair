#pragma once

#include <Geode/ui/GeodeUI.hpp>
#include <Geode/binding/GameLevelManager.hpp>
#include <fstream>
#include <functional>
#include "../Training.hpp"
#include "Widgets.hpp"

namespace roadui {
class AutoBackupPicker final : public Popup {
    std::vector<std::filesystem::path> m_files;
    std::function<void(std::filesystem::path)> m_select;
    void onClose(CCObject* sender) override { Popup::onClose(sender); }
    void onPick(CCObject* sender) {
        const auto index = static_cast<std::size_t>(sender->getTag());
        if (index >= m_files.size()) return;
        const auto file = m_files[index];
        if (m_select) m_select(file);
        Popup::onClose(nullptr);
    }
public:
    static void open(std::function<void(std::filesystem::path)> select) {
        auto popup = new AutoBackupPicker();
        if (!popup->init(420, 280)) { delete popup; return; }
        popup->autorelease();
        popup->m_files = training::backupFiles(training::autoBackupDir());
        popup->m_select = std::move(select);
        popup->m_bgSprite->setVisible(false);
        popup->m_closeBtn->setVisible(false);
        auto frame = panel(popup->m_mainLayer, 0, 0, 420, 280);
        frame->setZOrder(-1);
        popup->setTitle("SAUVEGARDES AUTO");
        auto header = menu(popup->m_mainLayer);
        button(header, popup, menu_selector(AutoBackupPicker::onClose), "X", 402, 263, 22, 22);
        label(popup->m_mainLayer, "10 versions maximum. Choisis une date a restaurer.",
              210, 224, 388, .27f, muted);
        auto list = ScrollLayer::create(CCSize{388, 172});
        list->setPosition({16, 40});
        const auto height = std::max(172.f, popup->m_files.size() * 32.f);
        list->m_contentLayer->setContentSize({388, height});
        popup->m_mainLayer->addChild(list);
        auto actions = menu(list->m_contentLayer);
        if (popup->m_files.empty())
            label(list->m_contentLayer, "Aucune copie automatique pour le moment.",
                  194, 86, 364, .27f, muted);
        for (std::size_t i = 0; i < popup->m_files.size(); ++i) {
            const auto name = popup->m_files[i].stem().string().substr(5);
            std::int64_t micros = 0;
            std::istringstream(name) >> micros;
            auto row = button(actions, popup, menu_selector(AutoBackupPicker::onPick),
                training::date(micros / 1000000), 194, height - 16 - i * 32.f, 386, 28);
            row->setTag(static_cast<int>(i));
        }
        list->scrollToTop();
        label(popup->m_mainLayer, "Une confirmation sera demandee avant restauration.",
              210, 22, 388, .24f, muted);
        fitPopup(popup->m_mainLayer, 420, 280);
        popup->show();
    }
};

class ToolsPopup final : public Popup {
    std::vector<int> m_levelIds;
    std::function<void()> m_changed;
    CCLabelBMFont* m_notice = nullptr;

    void notice(std::string const& text, bool error = false) {
        m_notice->setString(text.c_str());
        m_notice->limitLabelWidth(280, .27f, .14f);
        m_notice->setColor(error ? ccColor3B{240, 170, 140} : muted);
    }
    bool knownId(int id) const {
        return std::find(m_levelIds.begin(), m_levelIds.end(), id) != m_levelIds.end();
    }
    void onBackup(CCObject*) {
        if (!training::Store::get().writable || !training::Extras::get().writable) {
            notice("Restaure une copie valide avant d'exporter.", true);
            return;
        }
        const auto path = Mod::get()->getSaveDir() / "training-backup.json";
        std::ofstream stream(path, std::ios::binary);
        stream << training::backupEnvelope().dump();
        stream.close();
        if (stream.fail()) { notice("Impossible d'enregistrer la sauvegarde.", true); return; }
        FLAlertLayer::create("Sauvegarde creee",
            fmt::format("Sauvegarde complete enregistree ici :\n{}", path.string()).c_str(), "OK")->show();
        notice("Anciennes et nouvelles donnees exportees.");
    }
    void restore(std::filesystem::path const& path) {
        std::error_code error;
        const auto size = std::filesystem::file_size(path, error);
        if (error || size > 5000000) { notice("Sauvegarde absente ou trop volumineuse.", true); return; }
        std::ifstream stream(path, std::ios::binary);
        if (!stream) { notice("Impossible d'ouvrir la sauvegarde.", true); return; }
        const std::string raw((std::istreambuf_iterator<char>(stream)), {});
        auto json = matjson::parse(raw);
        const auto schema = json ? json.unwrap()["schema"].asInt().unwrapOr(0) : 0;
        if (schema != 1 && schema != 2) { notice("Format de sauvegarde non pris en charge.", true); return; }
        training::Store candidate;
        training::Extras extra;
        if (!candidate.decode(json.unwrap()["payload"].asString().unwrapOr("")) ||
            (candidate.goal && !knownId(candidate.goal)) ||
            (schema == 2 && !extra.decode(json.unwrap()["extras"].asString().unwrapOr("")))) {
            notice("Sauvegarde invalide. Donnees conservees.", true);
            return;
        }
        for (auto const& [id, value] : candidate.levels)
            if (!knownId(id)) { notice("Niveau hors du parcours dans la sauvegarde.", true); return; }
        auto& current = training::Store::get();
        const auto stamp = std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
        std::ofstream before(Mod::get()->getSaveDir() / ("training-before-import-" + stamp + ".json"),
                             std::ios::binary);
        before << training::backupEnvelope().dump();
        before.close();
        if (before.fail()) { notice("Copie de secours impossible. Import annule.", true); return; }
        for (int id : m_levelIds) {
            auto& best = candidate.levels[id].best;
            best = std::max(best, current.levels[id].best);
            if (auto saved = GameLevelManager::get()->getSavedLevel(id))
                best = std::max(best, std::clamp(saved->getNormalPercent(), 0, 100));
            if (schema == 2) {
                auto& value = extra.levels[id];
                value.seenBest = std::max(value.seenBest, best);
                if (value.goalKind == 1 && best >= value.goalTo) value.goalDone = true;
            }
        }
        // An old manual backup replaces only its legacy fields. New data is
        // retained; a full schema-2 backup replaces both independently validated payloads.
        current = std::move(candidate);
        current.save();
        if (schema == 2) {
            training::Extras::get() = std::move(extra);
            training::Extras::get().save();
        }
        notice(schema == 2 ? "Sauvegarde complete restauree." : "Ancien format restaure. Nouvelles donnees conservees.");
    }
    void confirmRestore(std::filesystem::path const& path) {
        Ref<ToolsPopup> self(this);
        createQuickPopup("Restaurer la sauvegarde ?",
            "Remplacer tes donnees par cette version ? Une copie de secours sera conservee. "
            "Tes records normaux ne diminueront pas.",
            "Annuler", "Restaurer", [self, path](FLAlertLayer*, bool confirmed) {
                if (confirmed) self->restore(path);
            });
    }
    void onRestore(CCObject*) { confirmRestore(Mod::get()->getSaveDir() / "training-backup.json"); }
    void onAutoBackups(CCObject*) {
        WeakRef<ToolsPopup> weak(this);
        AutoBackupPicker::open([weak](std::filesystem::path path) {
            if (auto self = weak.lock(); self && self->isRunning()) self->confirmRestore(path);
        });
    }
    void onSettings(CCObject*) { openSettingsPopup(Mod::get()); }
    void onClose(CCObject* sender) override {
        if (m_changed) m_changed();
        Popup::onClose(sender);
    }
    bool setup(std::vector<int> ids, std::function<void()> changed) {
        if (!Popup::init(316, 270)) return false;
        m_levelIds = std::move(ids);
        m_changed = std::move(changed);
        m_bgSprite->setVisible(false);
        m_closeBtn->setVisible(false);
        m_buttonMenu->setPosition({0, 0});
        m_buttonMenu->setAnchorPoint({0, 0});
        auto frame = panel(m_mainLayer, 0, 0, 316, 270);
        frame->setZOrder(-1);
        rect(frame, 1, 236, 314, 33, inset);
        label(m_mainLayer, "OUTILS", 158, 253, 220, .46f);
        button(m_buttonMenu, this, menu_selector(ToolsPopup::onClose), "X", 299, 253, 22, 22);
        label(m_mainLayer, "Tes donnees restent sur cet appareil.", 158, 219, 284, .28f, muted);
        button(m_buttonMenu, this, menu_selector(ToolsPopup::onBackup),
               "EXPORTER LA SAUVEGARDE", 158, 186, 264, 28, true);
        button(m_buttonMenu, this, menu_selector(ToolsPopup::onRestore),
               "RESTAURER LA SAUVEGARDE", 158, 148, 264, 28);
        button(m_buttonMenu, this, menu_selector(ToolsPopup::onAutoBackups),
               "SAUVEGARDES AUTO", 158, 110, 264, 28);
        button(m_buttonMenu, this, menu_selector(ToolsPopup::onSettings),
               "PARAMETRES DU MOD", 158, 72, 264, 28);
        m_notice = label(m_mainLayer, "10 versions auto. Export : training-backup.json.",
                         158, 30, 280, .25f, muted);
        fitPopup(m_mainLayer, 316, 270);
        return true;
    }
public:
    static ToolsPopup* create(std::vector<int> ids, std::function<void()> changed = {}) {
        auto popup = new ToolsPopup();
        if (popup->setup(std::move(ids), std::move(changed))) { popup->autorelease(); return popup; }
        delete popup;
        return nullptr;
    }
};
}
