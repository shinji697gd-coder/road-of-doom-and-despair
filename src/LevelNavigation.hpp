#pragma once

#include <Geode/binding/LevelBrowserLayer.hpp>
#include <Geode/binding/LevelInfoLayer.hpp>
#include <Geode/modify/LevelCell.hpp>
#include "OnlineLevels.hpp"
#include "Persistence.hpp"
#include "SearchQuery.hpp"
#include "ui/Widgets.hpp"

namespace training {
using namespace geode::prelude;
inline void searchCopies(int owner, std::string const& name) {
    auto search = GJSearchObject::create(SearchType::Search, road::startposQuery(name));
    auto scene = LevelBrowserLayer::scene(search);
    auto marker = CCNode::create();
    marker->setID("startpos-selection-owner"_spr);
    marker->setTag(owner);
    scene->addChild(marker);
    CCDirector::get()->pushScene(CCTransitionFade::create(.3f, scene));
}

class OpenCopyPopup final : public Popup {
    int m_copy = 0;
    std::string m_key;
    Ref<GJSearchObject> m_search;
    CCLabelBMFont* m_status = nullptr;
    bool m_open = true;
    void onClose(CCObject* sender) override {
        m_open = false;
        roadnet::replies.cancel(m_key);
        unscheduleAllSelectors();
        Popup::onClose(sender);
    }
    void fail(float = 0) {
        roadnet::replies.cancel(m_key);
        unscheduleAllSelectors();
        m_status->setString("Copie indisponible. Reessaie ou change son ID.");
        m_status->limitLabelWidth(270, .3f, .16f);
    }
    void finish(CCArray* levels) {
        Ref<GJGameLevel> chosen;
        if (levels) for (auto level : CCArrayExt<GJGameLevel*>(levels))
            if (level && static_cast<int>(level->m_levelID) == m_copy) chosen = level;
        if (!chosen) { fail(); return; }
        onClose(nullptr);
        CCDirector::get()->pushScene(
            CCTransitionFade::create(.3f, LevelInfoLayer::scene(chosen.data(), false)));
    }
    void retry(float) {
        Ref<OpenCopyPopup> keep(this);
        if (!m_open) return;
        if (auto cached = GameLevelManager::get()->getStoredOnlineLevels(m_key.c_str())) {
            finish(cached);
            return;
        }
        WeakRef<OpenCopyPopup> weak(this);
        if (roadnet::fetch(m_search.data(), [weak](roadnet::LevelReply const& result) {
            if (auto self = weak.lock(); self && self->m_open) {
                if (result.success) self->finish(result.levels.data());
                else self->fail();
            }
        })) unschedule(schedule_selector(OpenCopyPopup::retry));
    }
public:
    static void open(int copy) {
        auto popup = new OpenCopyPopup();
        if (!popup->init(310, 140)) { delete popup; return; }
        popup->autorelease();
        popup->m_copy = copy;
        popup->m_bgSprite->setVisible(false);
        popup->m_closeBtn->setVisible(false);
        auto frame = roadui::panel(popup->m_mainLayer, 0, 0, 310, 140);
        frame->setZOrder(-1);
        auto buttons = roadui::menu(popup->m_mainLayer);
        roadui::button(buttons, popup, menu_selector(OpenCopyPopup::onClose), "X", 292, 123, 22, 22);
        popup->setTitle("COPIE STARTPOS");
        popup->m_status = roadui::label(popup->m_mainLayer, "Chargement de ta copie...",
                                       155, 65, 270, .3f);
        popup->m_search = GJSearchObject::create(SearchType::Type19, std::to_string(copy));
        popup->m_key = popup->m_search->getKey();
        roadui::fitPopup(popup->m_mainLayer, 310, 140);
        popup->show();
        popup->scheduleOnce(schedule_selector(OpenCopyPopup::fail), 15.f);
        popup->schedule(schedule_selector(OpenCopyPopup::retry), .25f);
        popup->retry(0);
    }
};
}

// This chooser only appears in results explicitly opened from RDoD. Native
// searches and VIEW buttons elsewhere retain their original callback.
class $modify(RoadmapCopyChoice, LevelCell) {
    void onClick(cocos2d::CCObject* sender) {
        using namespace geode::prelude;
        auto scene = CCDirector::get()->getRunningScene();
        auto marker = scene ? scene->getChildByID("startpos-selection-owner"_spr) : nullptr;
        const int owner = marker ? marker->getTag() : 0;
        const int copy = m_level ? static_cast<int>(m_level->m_levelID) : 0;
        auto& extras = training::Extras::get();
        if (!training::knownLevel(owner) || !copy || training::knownLevel(copy) ||
            !extras.writable || extras.levels[owner].copy == copy) {
            LevelCell::onClick(sender);
            return;
        }
        Ref<GJGameLevel> level(m_level);
        createQuickPopup("Ta copie startpos ?",
            "Choisir cette copie pour ouvrir directement sa fiche depuis STARTPOS ?",
            "Voir seulement", "Choisir", [owner, copy, level](FLAlertLayer*, bool choose) {
                if (choose) {
                    auto& data = training::Extras::get();
                    if (!data.setCopy(owner, copy)) {
                        FLAlertLayer::create("Copie deja associee",
                            "Cette copie est deja liee a un autre niveau du parcours.", "OK")->show();
                        return;
                    }
                    data.save();
                }
                CCDirector::get()->pushScene(CCTransitionFade::create(.3f,
                    LevelInfoLayer::scene(level.data(), false)));
            });
    }
};
