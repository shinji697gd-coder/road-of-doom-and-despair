#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/ScrollLayer.hpp>
#include <functional>
#include "TrainingData.hpp"
#include "ui/Widgets.hpp"

namespace training {
using namespace geode::prelude;

inline void Store::save() {
    if (writable) Mod::get()->setSavedValue("training-v1", encode());
}

inline Store& Store::get() {
    static Store store;
    static bool loaded = false;
    if (!loaded) {
        loaded = true;
        auto raw = Mod::get()->getSavedValue<std::string>("training-v1", "");
        if (!raw.empty() && !store.decode(raw)) {
            store.writable = false;
            log::error("[RDoD] Invalid training save: writing disabled, original retained");
        }
    }
    return store;
}

// A page displays one task. Inputs and record lists are never drawn over the
// level browser, and native game progress is never edited from this popup.
class Detail final : public Popup {
    int m_id = 0;
    int m_tab = 0;
    int m_selectedRun = -1;
    int m_selectedPreset = -1;
    CCNode* m_body = nullptr;
    CCLabelBMFont* m_notice = nullptr;
    CCMenuItemSpriteExtra* m_tabs[3]{};
    TextInput* m_notes = nullptr;
    TextInput* m_name = nullptr;
    TextInput* m_from = nullptr;
    TextInput* m_to = nullptr;
    std::function<void()> m_changed;

    Progress& progress() { return Store::get().levels[m_id]; }

    void notice(std::string const& value, bool error = false) {
        m_notice->setString(value.c_str());
        m_notice->limitLabelWidth(380.f, .27f, .15f);
        m_notice->setColor(error ? ccColor3B{240, 170, 140} : roadui::muted);
    }

    bool canWrite() {
        if (Store::get().writable) return true;
        notice("Sauvegarde illisible : restaure une copie via Outils.", true);
        return false;
    }

    bool number(TextInput* input, double& value) {
        if (!input) return false;
        std::string text = input->getString();
        std::istringstream stream(text);
        if (!(stream >> value) || !std::isfinite(value)) return false;
        stream >> std::ws;
        return stream.eof();
    }

    void saveNotes(bool feedback = false) {
        if (!m_notes || !canWrite()) return;
        std::string value = m_notes->getString();
        if (progress().notes != value) {
            progress().notes = value;
            Store::get().save();
        }
        if (feedback) notice("Notes enregistrees.");
    }

    void onSaveNotes(CCObject*) { saveNotes(true); }

    void onStatus(CCObject*) {
        if (!canWrite()) return;
        saveNotes();
        progress().status = (progress().status + 1) % 4;
        Store::get().save();
        buildPage();
    }

    void onFavorite(CCObject*) {
        if (!canWrite()) return;
        saveNotes();
        progress().favorite = !progress().favorite;
        Store::get().save();
        buildPage();
    }

    void onGoal(CCObject*) {
        if (!canWrite()) return;
        saveNotes();
        auto& store = Store::get();
        store.goal = store.goal == m_id ? 0 : m_id;
        store.save();
        buildPage();
    }

    void onTab(CCObject* sender) {
        saveNotes();
        m_tab = sender->getTag();
        buildPage();
    }

    ScrollLayer* recordList(float bottom, float height, size_t count) {
        auto list = ScrollLayer::create(CCSize{388.f, height});
        list->setPosition({16.f, bottom});
        list->m_contentLayer->setContentSize({388.f, std::max(height, count * 26.f)});
        list->setID("training-record-list"_spr);
        m_body->addChild(list);
        return list;
    }

    void buildOverview() {
        auto& p = progress();
        auto best = roadui::panel(m_body, 16, 143, 112, 63, roadui::inset);
        roadui::label(best, "RECORD NORMAL", 56, 48, 100, .25f, roadui::muted);
        roadui::label(best, fmt::format("{}%", p.best), 56, 22, 100, .72f, roadui::green);

        auto options = roadui::menu(m_body);
        static constexpr char const* statuses[] = {
            "NON COMMENCE", "APPRENTISSAGE", "GRIND", "EN PAUSE"
        };
        roadui::button(options, this, menu_selector(Detail::onStatus),
                       p.best == 100 ? "TERMINE" : statuses[p.status],
                       203, 190, 130, 26);
        roadui::button(options, this, menu_selector(Detail::onFavorite),
                       p.favorite ? "EN FAVORIS" : "FAVORI",
                       338, 190, 130, 26, p.favorite);
        roadui::button(options, this, menu_selector(Detail::onGoal),
                       Store::get().goal == m_id ? "OBJECTIF ACTUEL" : "CHOISIR OBJECTIF",
                       203, 157, 130, 26, Store::get().goal == m_id);
        roadui::label(m_body, fmt::format("{} runs / {} startpos", p.runs.size(), p.presets.size()),
                      338, 157, 130, .26f, roadui::muted);

        roadui::label(m_body, "NOTES PERSONNELLES", 16, 126, 240, .29f, roadui::muted, true);
        m_notes = roadui::input(m_body, 210, 103, 388, "Note un timing, une transition ou un conseil...");
        m_notes->setMaxCharCount(4096);
        m_notes->setString(p.notes);
        roadui::button(options, this, menu_selector(Detail::onSaveNotes),
                       "ENREGISTRER", 80, 66, 128, 26, true);
        roadui::label(m_body, "Record lu dans Geometry Dash.",
                      279, 66, 242, .25f, roadui::muted);
    }

    void onSelectRun(CCObject* sender) {
        m_selectedRun = sender->getTag();
        buildPage();
    }

    void onClearRun(CCObject*) {
        m_selectedRun = -1;
        buildPage();
    }

    void onSaveRun(CCObject*) {
        if (!canWrite()) return;
        double from, to;
        if (!number(m_from, from) || !number(m_to, to) ||
            from < 0 || from >= to || to > 100) {
            notice("Utilise 0 <= Debut < Fin <= 100.", true);
            return;
        }
        auto& runs = progress().runs;
        if (m_selectedRun >= 0 && static_cast<size_t>(m_selectedRun) < runs.size()) {
            runs[m_selectedRun] = {from, to};
        } else {
            if (runs.size() >= 1000) {
                notice("Limite de runs atteinte.", true);
                return;
            }
            runs.emplace_back(from, to);
        }
        Store::get().save();
        m_selectedRun = -1;
        buildPage();
        notice("Run enregistre.");
    }

    void onDeleteRun(CCObject*) {
        if (!canWrite()) return;
        auto& runs = progress().runs;
        if (m_selectedRun < 0 || static_cast<size_t>(m_selectedRun) >= runs.size()) return;
        runs.erase(runs.begin() + m_selectedRun);
        m_selectedRun = -1;
        Store::get().save();
        buildPage();
        notice("Run supprime.");
    }

    void buildRuns() {
        auto& runs = progress().runs;
        const bool editing = m_selectedRun >= 0 && static_cast<size_t>(m_selectedRun) < runs.size();
        roadui::label(m_body, "DEBUT %", 62, 198, 90, .26f, roadui::muted);
        roadui::label(m_body, "FIN %", 166, 198, 90, .26f, roadui::muted);
        m_from = roadui::input(m_body, 62, 175, 92, "0", CommonFilter::Float);
        m_to = roadui::input(m_body, 166, 175, 92, "100", CommonFilter::Float);
        m_from->setMaxCharCount(6);
        m_to->setMaxCharCount(6);
        if (editing) {
            m_from->setString(fmt::format("{}", runs[m_selectedRun].first));
            m_to->setString(fmt::format("{}", runs[m_selectedRun].second));
        }
        auto actions = roadui::menu(m_body);
        roadui::button(actions, this, menu_selector(Detail::onSaveRun),
                       editing ? "ENREGISTRER" : "AJOUTER UN RUN", 316, 175, 176, 24, true);
        roadui::label(m_body, fmt::format("RUNS ENREGISTRES ({})", runs.size()),
                      16, 144, 220, .27f, roadui::muted, true);
        roadui::button(actions, this, menu_selector(Detail::onClearRun),
                       "NOUVEAU", 290, 143, 64, 20);
        auto remove = roadui::button(actions, this, menu_selector(Detail::onDeleteRun),
                                    "SUPPRIMER", 368, 143, 72, 20);
        remove->setEnabled(editing);
        remove->setOpacity(editing ? 255 : 95);

        auto list = recordList(51, 77, runs.size());
        if (runs.empty()) {
            roadui::label(list->m_contentLayer, "Tes runs enregistres apparaitront ici.",
                          194, 38, 360, .3f, roadui::muted);
        }
        auto rows = roadui::menu(list->m_contentLayer);
        const float height = list->m_contentLayer->getContentHeight();
        for (size_t i = 0; i < runs.size(); ++i) {
            auto row = roadui::button(rows, this, menu_selector(Detail::onSelectRun),
                                     fmt::format("{} - {}%", runs[i].first, runs[i].second),
                                     194, height - 13 - i * 26, 386, 24,
                                     static_cast<int>(i) == m_selectedRun);
            row->setTag(static_cast<int>(i));
        }
        list->scrollToTop();
    }

    void onSelectPreset(CCObject* sender) {
        m_selectedPreset = sender->getTag();
        buildPage();
    }

    void onNewPreset(CCObject*) {
        m_selectedPreset = -1;
        buildPage();
    }

    void onSavePreset(CCObject*) {
        if (!canWrite()) return;
        double percent;
        std::string name = m_name->getString();
        if (name.empty() || name.find_first_not_of(" \t\r\n") == std::string::npos ||
            !number(m_from, percent) || percent < 0 || percent >= 100) {
            notice("Indique un nom et un depart inferieur a 100 %.", true);
            return;
        }
        auto& presets = progress().presets;
        if (m_selectedPreset >= 0 && static_cast<size_t>(m_selectedPreset) < presets.size()) {
            presets[m_selectedPreset] = {name, percent};
        } else {
            if (presets.size() >= 1000) {
                notice("Limite de startpos atteinte.", true);
                return;
            }
            presets.push_back({name, percent});
        }
        Store::get().save();
        m_selectedPreset = -1;
        buildPage();
        notice("Startpos enregistre.");
    }

    void onDeletePreset(CCObject*) {
        if (!canWrite()) return;
        auto& presets = progress().presets;
        if (m_selectedPreset < 0 || static_cast<size_t>(m_selectedPreset) >= presets.size()) return;
        presets.erase(presets.begin() + m_selectedPreset);
        m_selectedPreset = -1;
        Store::get().save();
        buildPage();
        notice("Startpos supprime.");
    }

    void buildStartpos() {
        auto& presets = progress().presets;
        const bool editing = m_selectedPreset >= 0 && static_cast<size_t>(m_selectedPreset) < presets.size();
        roadui::label(m_body, "NOM DU STARTPOS", 16, 198, 230, .26f, roadui::muted, true);
        roadui::label(m_body, "DEPART %", 356, 198, 96, .26f, roadui::muted);
        m_name = roadui::input(m_body, 154, 175, 276, "Entrainement de la fin");
        m_from = roadui::input(m_body, 356, 175, 96, "72.8", CommonFilter::Float);
        m_name->setMaxCharCount(80);
        m_from->setMaxCharCount(6);
        if (editing) {
            m_name->setString(presets[m_selectedPreset].name);
            m_from->setString(fmt::format("{}", presets[m_selectedPreset].percent));
        }
        auto actions = roadui::menu(m_body);
        roadui::button(actions, this, menu_selector(Detail::onNewPreset),
                       "NOUVEAU", 52, 143, 72, 22);
        auto remove = roadui::button(actions, this, menu_selector(Detail::onDeletePreset),
                                    "SUPPRIMER", 138, 143, 84, 22);
        remove->setEnabled(editing);
        remove->setOpacity(editing ? 255 : 95);
        roadui::button(actions, this, menu_selector(Detail::onSavePreset),
                       editing ? "ENREGISTRER" : "AJOUTER UN STARTPOS", 303, 143, 202, 22, true);

        auto list = recordList(57, 71, presets.size());
        if (presets.empty()) {
            roadui::label(list->m_contentLayer, "Aucun startpos enregistre pour ce niveau.",
                          194, 35, 360, .3f, roadui::muted);
        }
        auto rows = roadui::menu(list->m_contentLayer);
        const float height = list->m_contentLayer->getContentHeight();
        for (size_t i = 0; i < presets.size(); ++i) {
            auto row = roadui::button(rows, this, menu_selector(Detail::onSelectPreset),
                                     fmt::format("{}  /  {}%", presets[i].name, presets[i].percent),
                                     194, height - 13 - i * 26, 386, 24,
                                     static_cast<int>(i) == m_selectedPreset);
            row->setTag(static_cast<int>(i));
        }
        list->scrollToTop();
        roadui::label(m_body, "Reperes uniquement : lancement par startpos indisponible.",
                      210, 43, 388, .23f, roadui::muted);
    }

    void buildPage() {
        m_notes = m_name = m_from = m_to = nullptr;
        m_body->removeAllChildrenWithCleanup(true);
        static constexpr char const* names[] = {"APERCU", "RUNS", "STARTPOS"};
        for (int i = 0; i < 3; ++i)
            roadui::updateButton(m_tabs[i], names[i], i == m_tab);
        notice(Store::get().writable ? "Enregistre sur cet appareil." : "Lecture seule : restaure une copie via Outils.",
               !Store::get().writable);
        if (m_tab == 0) buildOverview();
        else if (m_tab == 1) buildRuns();
        else buildStartpos();
    }

    void onClose(CCObject* sender) override {
        saveNotes();
        if (m_changed) m_changed();
        Popup::onClose(sender);
    }

    bool setup(int id, std::string const& name, int tab, std::function<void()> changed) {
        if (!Popup::init(420.f, 280.f)) return false;
        m_id = id;
        m_tab = std::clamp(tab, 0, 2);
        m_changed = std::move(changed);
        m_bgSprite->setVisible(false);
        m_closeBtn->setVisible(false);
        m_buttonMenu->setPosition({0, 0});
        m_buttonMenu->setAnchorPoint({0, 0});
        auto frame = roadui::panel(m_mainLayer, 0, 0, 420, 280);
        frame->setZOrder(-1);
        roadui::rect(frame, 1, 242, 418, 37, roadui::inset);
        roadui::label(m_mainLayer, name, 210, 265, 330, .52f);
        roadui::label(m_mainLayer, fmt::format("NIVEAU {}", id), 210, 247, 300, .23f, roadui::muted);
        roadui::button(m_buttonMenu, this, menu_selector(Detail::onClose), "X",
                       402, 263, 22, 22);
        static constexpr char const* names[] = {"APERCU", "RUNS", "STARTPOS"};
        for (int i = 0; i < 3; ++i) {
            m_tabs[i] = roadui::button(m_buttonMenu, this, menu_selector(Detail::onTab),
                                      names[i], 85.f + 125.f * i, 226, 112, 23, i == m_tab);
            m_tabs[i]->setTag(i);
        }
        m_body = CCNode::create();
        m_mainLayer->addChild(m_body);
        m_notice = roadui::label(m_mainLayer, "", 210, 20, 380, .27f, roadui::muted);
        roadui::fitPopup(m_mainLayer, 420, 280);
        buildPage();
        return true;
    }

public:
    static Detail* create(int id, std::string const& name, int tab = 0,
                          std::function<void()> changed = {}) {
        auto popup = new Detail();
        if (popup->setup(id, name, tab, std::move(changed))) {
            popup->autorelease();
            return popup;
        }
        delete popup;
        return nullptr;
    }
};
}
