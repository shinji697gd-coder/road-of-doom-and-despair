#pragma once

#include "../Persistence.hpp"
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
        struct Row { std::int64_t time; std::string first, second; };
        std::vector<Row> rows;
        std::array<std::int64_t, 2> attempts{};
        std::array<double, 2> seconds{};
        if (m_tab == 2) {
            for (auto const& [owner, value] : data.levels) {
                if (m_owner && owner != m_owner) continue;
                for (auto const& record : value.records)
                    rows.push_back({record.observed,
                        fmt::format("{}%  /  {}", record.best, training::date(record.observed)),
                        m_owner ? "Record normal observe sur cet appareil" : fmt::format("Niveau {}", owner)});
            }
        } else {
            for (auto const& session : data.sessions) {
                if ((m_owner && session.owner != m_owner) ||
                    (session.played != session.owner) != (m_tab == 1)) continue;
                if (session.attempts[0] + session.attempts[1] == 0) continue;
                for (int i = 0; i < 2; ++i) {
                    attempts[i] += session.attempts[i];
                    seconds[i] += session.seconds[i];
                }
                rows.push_back({session.started,
                    fmt::format("{} / ID {}", training::date(session.started), session.played),
                    fmt::format("{} normaux / {} practice/test  |  {}", session.attempts[0],
                        session.attempts[1], training::duration(session.seconds[0] + session.seconds[1]))});
            }
        }
        std::stable_sort(rows.begin(), rows.end(), [](Row const& a, Row const& b) { return a.time > b.time; });
        label(m_body, m_tab == 2 ? "Dates d'observation, pas les dates des anciens records." :
              fmt::format("{} essais normaux / {} practice/test", attempts[0], attempts[1]),
              210, 193, 388, .27f, muted);
        label(m_body, m_tab == 2 ? fmt::format("{} nouveaux records suivis", rows.size()) :
              fmt::format("Temps actif : {} / practice : {}", training::duration(seconds[0]),
                          training::duration(seconds[1])),
              210, 171, 388, .29f, green);
        auto list = ScrollLayer::create(CCSize{388, 116});
        list->setPosition({16, 40});
        const float height = std::max(116.f, rows.size() * 44.f);
        list->m_contentLayer->setContentSize({388, height});
        m_body->addChild(list);
        if (rows.empty()) label(list->m_contentLayer, "Aucune donnee depuis l'activation du suivi.",
                                194, 58, 366, .28f, muted);
        for (std::size_t i = 0; i < rows.size(); ++i) {
            auto row = panel(list->m_contentLayer, 0, height - (i + 1) * 44.f, 388, 42, inset);
            label(row, rows[i].first, 10, 29, 366, .28f, text, true);
            label(row, rows[i].second, 10, 12, 366, .24f, muted, true);
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
