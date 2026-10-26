#pragma once

#include "../Persistence.hpp"
#include "../StatsData.hpp"
#include "Widgets.hpp"
#include <Geode/ui/ScrollLayer.hpp>

namespace roadui {
class StatsPopup final : public Popup {
    int m_owner = 0, m_tab = 0;
    CCNode* m_body = nullptr;
    CCMenuItemSpriteExtra* m_tabs[3]{};
    void onClose(CCObject* sender) override { Popup::onClose(sender); }
    void onTab(CCObject* sender) { m_tab = sender->getTag(); build(); }
    void build() {
        m_body->removeAllChildrenWithCleanup(true);
        static const char* captions[] = {"ORIGINAL", "COPIES", "RECORDS"};
        for (int i = 0; i < 3; ++i) updateButton(m_tabs[i], captions[i], m_tab == i);
        auto& data = training::Extras::get();
        const auto view = training::statsView(data, m_owner, m_tab);
        label(m_body, m_tab == 2 ? "Dates d'observation, pas les dates des anciens records." :
              fmt::format("{} essais normaux / {} practice/test", view.attempts[0], view.attempts[1]),
              210, 193, 388, .27f, muted);
        label(m_body, m_tab == 2 ? fmt::format("{} nouveaux records suivis", view.rows.size()) :
              fmt::format("Temps actif : {} / practice : {}", training::duration(view.seconds[0]),
                          training::duration(view.seconds[1])),
              210, 171, 388, .29f, green);
        auto list = ScrollLayer::create(CCSize{388, 116});
        list->setPosition({16, 40});
        constexpr float rowHeight = 60;
        const float height = std::max(116.f, view.rows.size() * rowHeight);
        list->m_contentLayer->setContentSize({388, height});
        m_body->addChild(list);
        if (view.rows.empty()) {
            const auto message = m_tab == 2 ? "Aucun nouveau record observe pour le moment." :
                !data.writable ? "Sauvegarde en lecture seule : restaure une copie valide." :
                !Mod::get()->getSettingValue<bool>("auto-session-stats") ?
                    "Active les stats de session dans les parametres du mod." :
                m_tab == 1 ? "Lie une copie via i / STARTPOS, puis joue dessus." :
                    "Joue un niveau du parcours, puis reviens consulter ses stats.";
            label(list->m_contentLayer, message, 194, 58, 366, .27f, muted);
        }
        for (std::size_t i = 0; i < view.rows.size(); ++i) {
            const auto& value = view.rows[i];
            auto row = panel(list->m_contentLayer, 0, height - (i + 1) * rowHeight,
                             388, rowHeight - 2, inset);
            label(row, m_tab == 2 ? fmt::format("{} / {}%", value.name, value.best) : value.name,
                  10, 44, 366, .32f, text, true);
            label(row, training::date(value.time) +
                  (m_tab == 2 ? " / record normal" : value.copy ? " / copie startpos" : " / original"),
                  10, 27, 366, .24f, muted, true);
            label(row, m_tab == 2 ? "Progression normale observee sur cet appareil" :
                  fmt::format("{} normaux / {} practice-test | {}", value.attempts[0],
                      value.attempts[1], training::duration(value.seconds[0] + value.seconds[1])),
                  10, 11, 366, .23f, muted, true);
        }
        list->scrollToTop();
    }
public:
    static void open(int owner = 0) {
        auto popup = new StatsPopup();
        if (!popup->init(420, 280)) { delete popup; return; }
        popup->autorelease();
        popup->m_owner = owner;
        popup->m_bgSprite->setVisible(false);
        popup->m_closeBtn->setVisible(false);
        auto frame = panel(popup->m_mainLayer, 0, 0, 420, 280);
        frame->setZOrder(-1);
        popup->setTitle("STATS ET HISTORIQUE");
        auto buttons = menu(popup->m_mainLayer);
        button(buttons, popup, menu_selector(StatsPopup::onClose), "X", 402, 263, 22, 22);
        static const char* captions[] = {"ORIGINAL", "COPIES", "RECORDS"};
        for (int i = 0; i < 3; ++i) {
            popup->m_tabs[i] = button(buttons, popup, menu_selector(StatsPopup::onTab),
                captions[i], 80 + 130.f * i, 226, 120, 24, i == 0);
            popup->m_tabs[i]->setTag(i);
        }
        popup->m_body = CCNode::create();
        popup->m_mainLayer->addChild(popup->m_body);
        label(popup->m_mainLayer, "Suivi local. Pauses exclues. 500 sessions maximum.",
              210, 22, 388, .24f, muted);
        fitPopup(popup->m_mainLayer, 420, 280);
        popup->build();
        popup->show();
    }
};
}
