#pragma once

#include "../Persistence.hpp"
#include "Widgets.hpp"
#include <functional>

namespace roadui {
class GoalPopup final : public Popup {
    int m_owner = 0, m_kind = 1;
    TextInput* m_from = nullptr;
    TextInput* m_to = nullptr;
    CCLabelBMFont* m_notice = nullptr;
    CCMenuItemSpriteExtra* m_kinds[2]{};
    std::function<void()> m_changed;
    void onClose(CCObject* sender) override { Popup::onClose(sender); }
    void onKind(CCObject* sender) {
        m_kind = sender->getTag();
        for (int i = 0; i < 2; ++i)
            updateButton(m_kinds[i], i == 0 ? "RECORD NORMAL" : "RUN", m_kind == i + 1);
        m_from->setEnabled(m_kind == 2);
        if (m_kind == 1) m_from->setString("0");
    }
    void onSave(CCObject*) {
        if (!training::Extras::get().writable) {
            m_notice->setString("Lecture seule : restaure une sauvegarde valide.");
            return;
        }
        auto parse = [](std::string raw, double& value) {
            std::replace(raw.begin(), raw.end(), ',', '.');
            std::istringstream input(raw);
            if (!(input >> value) || !std::isfinite(value)) return false;
            input >> std::ws;
            return input.eof();
        };
        double from, to;
        if (!parse(m_from->getString(), from) || !parse(m_to->getString(), to) ||
            from < 0 || from >= to || to > 100 ||
            (m_kind == 1 && (from != 0 || std::floor(to) != to))) {
            m_notice->setString("Utilise 0 <= Debut < Fin <= 100 (record entier).");
            m_notice->limitLabelWidth(372, .25f, .16f);
            return;
        }
        auto& data = training::Extras::get();
        auto& level = data.levels[m_owner];
        level.goalKind = m_kind;
        level.goalFrom = from; level.goalTo = to;
        level.goalDone = m_kind == 1 && training::Store::get().levels[m_owner].best >= to;
        if (m_kind == 2)
            for (auto [a, b] : training::Store::get().levels[m_owner].runs)
                data.observeRun(m_owner, a, b);
        data.save();
        if (m_changed) m_changed();
        Popup::onClose(nullptr);
    }
    void onRemove(CCObject*) {
        auto& data = training::Extras::get();
        if (!data.writable) return;
        auto& level = data.levels[m_owner];
        level.goalKind = 0; level.goalFrom = 0; level.goalTo = 100; level.goalDone = false;
        data.save();
        if (m_changed) m_changed();
        Popup::onClose(nullptr);
    }
public:
    static void open(int owner, std::function<void()> changed = {}) {
        auto popup = new GoalPopup();
        if (!popup->init(420, 250)) { delete popup; return; }
        popup->autorelease();
        popup->m_owner = owner;
        popup->m_changed = std::move(changed);
        popup->m_bgSprite->setVisible(false);
        popup->m_closeBtn->setVisible(false);
        auto frame = panel(popup->m_mainLayer, 0, 0, 420, 250);
        frame->setZOrder(-1);
        popup->setTitle("OBJECTIF PRECIS");
        auto buttons = menu(popup->m_mainLayer);
        button(buttons, popup, menu_selector(GoalPopup::onClose), "X", 402, 233, 22, 22);
        const auto& goal = training::Extras::get().levels[owner];
        popup->m_kind = goal.goalKind == 2 ? 2 : 1;
        for (int i = 0; i < 2; ++i) {
            popup->m_kinds[i] = button(buttons, popup, menu_selector(GoalPopup::onKind),
                i == 0 ? "RECORD NORMAL" : "RUN", 111 + i * 198.f, 195, 186, 26,
                popup->m_kind == i + 1);
            popup->m_kinds[i]->setTag(i + 1);
        }
        label(popup->m_mainLayer, "DEBUT %", 111, 164, 170, .28f, muted);
        label(popup->m_mainLayer, "FIN %", 309, 164, 170, .28f, muted);
        popup->m_from = input(popup->m_mainLayer, 111, 139, 186, "0", CommonFilter::Float);
        popup->m_to = input(popup->m_mainLayer, 309, 139, 186, "70", CommonFilter::Float);
        popup->m_from->setMaxCharCount(6); popup->m_to->setMaxCharCount(6);
        popup->m_from->setString(fmt::format("{}", goal.goalFrom));
        popup->m_to->setString(fmt::format("{}", goal.goalKind ? goal.goalTo : 70));
        popup->m_from->setEnabled(popup->m_kind == 2);
        label(popup->m_mainLayer, "Record : progression GD. Run : tes runs enregistres.",
              210, 107, 388, .25f, muted);
        button(buttons, popup, menu_selector(GoalPopup::onRemove), "RETIRER", 111, 73, 186, 28);
        button(buttons, popup, menu_selector(GoalPopup::onSave), "ENREGISTRER", 309, 73, 186, 28, true);
        popup->m_notice = label(popup->m_mainLayer, "Exemple : 70% en normal ou un run de 40 a 100%.",
                               210, 33, 388, .25f, muted);
        fitPopup(popup->m_mainLayer, 420, 250);
        popup->show();
    }
};
}
