#include <Geode/Geode.hpp>
#include <Geode/modify/LevelSearchLayer.hpp>
#include <Geode/binding/GameLevelManager.hpp>
#include <Geode/binding/GJSearchObject.hpp>
#include <Geode/binding/LevelCell.hpp>
#include <Geode/binding/LevelBrowserLayer.hpp>
#include <Geode/ui/BasedButtonSprite.hpp>
#include <Geode/ui/ScrollLayer.hpp>
#include <array>
#include <cctype>
#include <map>
#include "level_data.hpp"
#include "Training.hpp"
#include "ui/Layout.hpp"
#include "ui/ToolsPopup.hpp"
#include "OnlineLevels.hpp"
#include "SearchQuery.hpp"
#include "SessionStats.hpp"

using namespace geode::prelude;

namespace {
struct Entry {
    std::string name;
    int id;
};

std::vector<Entry> getRoadmap() {
    std::vector<Entry> result;
    std::istringstream source(road::levels);
    std::string line;
    while (std::getline(source, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const auto separator = line.find('|');
        if (separator != std::string::npos)
            result.push_back({line.substr(0, separator), std::stoi(line.substr(separator + 1))});
    }
    return result;
}

std::string lower(std::string value) {
    for (auto& c : value)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return value;
}

constexpr char const* filters[] = {"TOUS", "A FAIRE", "TERMINES", "FAVORIS", "OBJECTIF"};
}

class RoadmapListLayer final : public CCLayer, public LevelManagerDelegate {
    struct ViewState {
        std::string query;
        unsigned page = 0;
        unsigned filter = 0;
    };
    std::vector<Entry> m_roadmap;
    std::vector<size_t> m_results;
    std::map<int, Ref<GJGameLevel>> m_cache;
    std::array<ViewState, 2> m_views;
    int m_tab = 0;
    bool m_inFlight = false;
    bool m_forceRefresh = false;
    unsigned m_generation = 0;
    unsigned m_requestGeneration = 0;
    std::string m_requestKey;
    std::vector<int> m_requestedIds;
    Ref<GJSearchObject> m_requestSearch;
    std::string m_networkMessage;

    CCNode* m_window = nullptr;
    CCNode* m_content = nullptr;
    CCNode* m_toolbar = nullptr;
    ScrollLayer* m_scroll = nullptr;
    TextInput* m_search = nullptr;
    CCLabelBMFont* m_progress = nullptr;
    CCLabelBMFont* m_pageLabel = nullptr;
    CCLabelBMFont* m_dashboardHeading = nullptr;
    CCMenuItemSpriteExtra* m_tabs[3]{};
    CCMenuItemSpriteExtra* m_filter = nullptr;
    CCMenuItemSpriteExtra* m_previous = nullptr;
    CCMenuItemSpriteExtra* m_next = nullptr;
    CCMenuItemSpriteExtra* m_statsButton = nullptr;

    static constexpr size_t pageSize = 10;

    ViewState& view() { return m_views[m_tab == 1 ? 1 : 0]; }

    bool knownId(int id) const {
        return std::any_of(m_roadmap.begin(), m_roadmap.end(),
                           [id](Entry const& entry) { return entry.id == id; });
    }

    void syncProgress() {
        if (!Mod::get()->getSettingValue<bool>("auto-sync-progress")) return;
        auto& store = training::Store::get();
        auto manager = GameLevelManager::get();
        bool changed = false;
        auto& extras = training::Extras::get();
        bool extraChanged = false;
        for (auto const& entry : m_roadmap) {
            int best = store.levels[entry.id].best;
            if (auto saved = manager->getSavedLevel(entry.id))
                best = std::max(best, std::clamp(saved->getNormalPercent(), 0, 100));
            if (auto cached = m_cache.find(entry.id); cached != m_cache.end())
                best = std::max(best, std::clamp(cached->second->getNormalPercent(), 0, 100));
            if (best != store.levels[entry.id].best) {
                store.levels[entry.id].best = best;
                changed = true;
            }
            if (extras.writable)
                extraChanged |= extras.observeBest(entry.id, best, training::now());
        }
        if (changed) store.save();
        if (extraChanged) extras.save();
    }

    void rebuildResults(bool resetPage = false) {
        m_results.clear();
        auto& state = view();
        auto& store = training::Store::get();
        if (resetPage) state.page = 0;
        for (size_t i = 0; i < m_roadmap.size(); ++i) {
            const auto& entry = m_roadmap[i];
            const auto& p = store.levels[entry.id];
            if (state.filter == 1 && p.best == 100) continue;
            if (state.filter == 2 && p.best != 100) continue;
            if (state.filter == 3 && !p.favorite) continue;
            if (state.filter == 4 && store.goal != entry.id) continue;
            if (lower(entry.name).find(state.query) != std::string::npos ||
                std::to_string(entry.id).find(state.query) != std::string::npos)
                m_results.push_back(i);
        }
        const unsigned last = m_results.empty() ? 0 :
            static_cast<unsigned>((m_results.size() - 1) / pageSize);
        state.page = std::min(state.page, last);
    }

    void updateChrome() {
        static constexpr char const* names[] = {"PARCOURS", "STARTPOS", "PROGRESSION"};
        for (int i = 0; i < 3; ++i)
            roadui::updateButton(m_tabs[i], names[i], m_tab == i);
        m_toolbar->setVisible(m_tab != 2);
        m_dashboardHeading->setVisible(m_tab == 2);
        m_statsButton->setVisible(m_tab == 2);
        if (m_tab != 2) {
            roadui::updateButton(m_filter, filters[view().filter]);
        }
        const auto& store = training::Store::get();
        const auto done = std::count_if(m_roadmap.begin(), m_roadmap.end(),
            [&store](Entry const& entry) { return store.levels.at(entry.id).best == 100; });
        m_progress->setString(fmt::format("{} / 75 termines", done).c_str());
        m_progress->limitLabelWidth(100, .28f, .18f);

        const bool hasPages = m_tab != 2 && !m_results.empty();
        const auto pages = std::max<size_t>(1, (m_results.size() + pageSize - 1) / pageSize);
        m_pageLabel->setVisible(hasPages);
        if (hasPages)
            m_pageLabel->setString(fmt::format("PAGE {} / {}", view().page + 1, pages).c_str());
        m_previous->setVisible(hasPages && view().page > 0);
        m_next->setVisible(hasPages && view().page + 1 < pages);
    }

    void clearContent() {
        m_scroll = nullptr;
        m_content->removeAllChildrenWithCleanup(true);
        roadui::rect(m_content, 0, 0, roadui::layout::contentWidth,
                     roadui::layout::contentHeight, roadui::inset);
    }

    void renderEmpty() {
        using namespace roadui::layout;
        clearContent();
        roadui::label(m_content, "AUCUN RESULTAT",
                      contentWidth / 2, 110, contentWidth - 28, .44f);
        roadui::label(m_content, "Essaie un autre nom, ID ou filtre.",
                      contentWidth / 2, 83, contentWidth - 28, .27f, roadui::muted);
        auto actions = roadui::menu(m_content);
        roadui::button(actions, this, menu_selector(RoadmapListLayer::onResetSearch),
                       "TOUT AFFICHER",
                       contentWidth / 2, 46, 188, 26, true);
        updateChrome();
    }

    void openDetail(size_t index, int tab) {
        if (index >= m_roadmap.size()) return;
        syncProgress();
        const auto& entry = m_roadmap[index];
        Ref<RoadmapListLayer> self(this);
        auto popup = training::Detail::create(entry.id, entry.name, tab, [self] {
            if (self->isRunning()) self->refreshLocal();
        });
        if (popup) popup->show();
    }

    void onDetail(CCObject* sender) {
        openDetail(static_cast<size_t>(sender->getTag()), m_tab == 1 ? 2 : 0);
    }

    void onView(CCObject* sender) {
        const auto index = static_cast<size_t>(sender->getTag());
        if (index >= m_roadmap.size()) return;
        const auto& entry = m_roadmap[index];
        // Building the native browser can start its request before our onExit.
        stopRequest();
        if (m_tab == 1) {
            const int copy = training::Extras::get().levels[entry.id].copy;
            if (copy) training::OpenCopyPopup::open(copy);
            else training::searchCopies(entry.id, entry.name);
            return;
        }
        const auto query = std::to_string(entry.id);
        auto search = GJSearchObject::create(SearchType::Search, query);
        CCDirector::get()->pushScene(
            CCTransitionFade::create(.3f, LevelBrowserLayer::scene(search)));
    }

    void renderDashboard() {
        using namespace roadui::layout;
        clearContent();
        auto& store = training::Store::get();
        int done = 0, favorites = 0, runs = 0, presets = 0;
        std::string goal = "Choisis un objectif";
        int goalBest = 0;
        for (auto const& entry : m_roadmap) {
            const auto& p = store.levels[entry.id];
            done += p.best == 100;
            favorites += p.favorite;
            runs += static_cast<int>(p.runs.size());
            presets += training::Extras::get().levels[entry.id].copy != 0;
            if (entry.id == store.goal) { goal = entry.name; goalBest = p.best; }
        }
        auto complete = roadui::panel(m_content, 0, 103, 138, 73);
        roadui::label(complete, "TERMINES", 69, 55, 122, .27f, roadui::muted);
        roadui::label(complete, fmt::format("{} / 75", done), 69, 26, 122, .64f, roadui::green);
        auto goalCard = roadui::panel(m_content, 146, 103, 286, 73);
        roadui::label(goalCard, "OBJECTIF ACTUEL", 12, 55, 260, .27f, roadui::muted, true);
        roadui::label(goalCard, goal, 12, 31, 260, .44f, roadui::text, true);
        const auto precise = store.goal && training::Extras::get().levels[store.goal].goalKind;
        roadui::label(goalCard, precise ? training::goalCaption(store.goal) :
                      store.goal ? fmt::format("Record normal : {}%", goalBest) :
                      "Choisis-le dans l'apercu d'un niveau.",
                      12, 12, 260, .24f, roadui::muted, true);
        roadui::rect(m_content, 0, 85, contentWidth, 8, roadui::surface);
        roadui::rect(m_content, 0, 85, contentWidth * done / 75.f, 8, roadui::selectedFill);

        const char* labels[] = {"FAVORIS", "RUNS", "COPIES LIEES"};
        const int values[] = {favorites, runs, presets};
        for (int i = 0; i < 3; ++i) {
            auto card = roadui::panel(m_content, i * 146.f, 10, 140, 63);
            roadui::label(card, labels[i], 70, 45, 126, .27f, roadui::muted);
            roadui::label(card, std::to_string(values[i]), 70, 20, 126, .6f);
        }
        updateChrome();
    }

    void renderRoadmap() {
        using namespace roadui::layout;
        clearContent();
        if (m_results.empty()) { renderEmpty(); return; }
        const auto begin = view().page * pageSize;
        const auto end = std::min(m_results.size(), begin + pageSize);
        const auto count = end - begin;
        const float height = std::max(contentHeight, count * rowHeight);
        m_scroll = ScrollLayer::create(CCSize{contentWidth, contentHeight});
        m_scroll->setID("roadmap-scroll"_spr);
        m_scroll->m_contentLayer->setContentSize({contentWidth, height});
        m_content->addChild(m_scroll);
        for (size_t i = begin; i < end; ++i) {
            const size_t index = m_results[i];
            const auto& entry = m_roadmap[index];
            const float y = height - (i - begin + 1) * rowHeight;
            auto row = CCNode::create();
            row->setContentSize({contentWidth, rowHeight});
            row->setPosition({0, y});
            row->setID(fmt::format("roadmap-row-{}"_spr, entry.id));
            m_scroll->m_contentLayer->addChild(row);
            auto cached = m_cache.find(entry.id);
            GJGameLevel* level = cached == m_cache.end() ? nullptr : cached->second.data();
            bool hasCell = false;
            if (level) {
                auto cell = LevelCell::create(nativeCellWidth, rowHeight);
                if (cell) {
                    cell->setContentSize({nativeCellWidth, rowHeight});
                    cell->setAnchorPoint({0, 0});
                    cell->setPosition({0, 0});
                    cell->loadFromLevel(level);
                    if (auto menu = cell->m_mainMenu) {
                        menu->setPosition({0, 0});
                        if (auto creator = menu->getChildByID("creator-name")) {
                            creator->setAnchorPoint({0, .5f});
                            creator->setPosition({50, 53});
                        }
                        auto view = typeinfo_cast<CCMenuItemSpriteExtra*>(menu->getChildByID("view-button"));
                        if (!view) view = cell->m_button;
                        if (view) {
                            view->setPosition({nativeCellWidth - 43, rowHeight * .5f});
                            if (m_tab == 1) {
                                view->setTag(static_cast<int>(index));
                                view->setTarget(this, menu_selector(RoadmapListLayer::onView));
                            }
                        }
                    }
                    cell->updateBGColor(static_cast<int>(i - begin));
                    row->addChild(cell);
                    hasCell = true;
                }
            }
            if (!hasCell) {
                roadui::rect(row, 0, 0, nativeCellWidth, rowHeight,
                             (i - begin) % 2 ? ccColor4B{65, 57, 49, 255} :
                                              ccColor4B{56, 49, 44, 255});
                roadui::label(row, entry.name, 14, 59, nativeCellWidth - 108, .49f,
                              roadui::text, true);
                roadui::label(row, fmt::format("ID {}  /  {}", entry.id,
                              m_inFlight ? std::string("Chargement...") :
                              (m_networkMessage.empty() ? "Infos indisponibles - Actualise pour reessayer" : m_networkMessage)),
                              14, 29, nativeCellWidth - 108, .26f, roadui::muted, true);
                // VIEW remains usable even if metadata is slow or unavailable.
                auto actions = roadui::menu(row);
                auto sprite = ButtonSprite::create("VIEW");
                sprite->setScale(.55f);
                auto view = CCMenuItemSpriteExtra::create(sprite, this,
                    menu_selector(RoadmapListLayer::onView));
                view->setPosition({nativeCellWidth - 43, rowHeight * .5f});
                view->setTag(static_cast<int>(index));
                view->setID("view-button");
                actions->addChild(view);
            }
            // RDoD owns a narrow gutter outside the native cell. Nothing is
            // inserted over its title, completion badge, VIEW or thumbnail.
            roadui::rect(row, nativeCellWidth, 0, detailGutter, rowHeight, roadui::surface);
            roadui::rect(row, nativeCellWidth, 0, 1, rowHeight, roadui::outline);
            auto actions = roadui::menu(row);
            auto detail = roadui::button(actions, this, menu_selector(RoadmapListLayer::onDetail),
                                         "i", nativeCellWidth + detailGutter / 2,
                                         rowHeight / 2, 22, 26);
            detail->setTag(static_cast<int>(index));
            detail->setID("roadmap-details-button"_spr);
            const auto& p = training::Store::get().levels[entry.id];
            roadui::label(row, fmt::format("{}%", p.best),
                          nativeCellWidth + detailGutter / 2, 73,
                          detailGutter - 2, .21f, roadui::muted);
            if (p.favorite)
                roadui::label(row, "*", nativeCellWidth + detailGutter / 2, 17,
                              24, .38f, roadui::green);
        }
        m_scroll->scrollToTop();
        updateChrome();
    }

    void render() {
        if (m_tab == 2) renderDashboard();
        else renderRoadmap();
    }

    void refreshLocal() {
        syncProgress();
        rebuildResults();
        render();
    }

    void stopRequest() {
        unschedule(schedule_selector(RoadmapListLayer::onRequestTimeout));
        unschedule(schedule_selector(RoadmapListLayer::onRequestRetry));
        roadnet::replies.cancel(m_requestKey);
        m_inFlight = false;
        m_requestKey.clear();
        m_requestedIds.clear();
        m_requestSearch = nullptr;
    }

    void onRequestTimeout(float) {
        const bool stale = m_requestGeneration != m_generation;
        stopRequest();
        m_networkMessage = "Delai depasse. Actualise pour reessayer.";
        render();
        if (stale && m_tab != 2) loadPage();
    }

    void onRequestRetry(float) {
        if (!m_inFlight || !m_requestSearch) return;
        const std::string key = m_requestKey;
        const auto generation = m_requestGeneration;
        if (auto cached = GameLevelManager::get()->getStoredOnlineLevels(key.c_str())) {
            loadLevelsFinished(cached, key.c_str(), 0);
            return;
        }
        WeakRef<RoadmapListLayer> weak(this);
        if (roadnet::fetch(m_requestSearch.data(), [weak, key](roadnet::LevelReply const& reply) {
            auto self = weak.lock();
            if (!self || !self->isRunning()) return;
            if (reply.success) self->loadLevelsFinished(reply.levels.data(), key.c_str(), 0);
            else self->loadLevelsFailed(key.c_str(), 0);
        }) && m_inFlight && m_requestKey == key && m_requestGeneration == generation) {
            unschedule(schedule_selector(RoadmapListLayer::onRequestRetry));
        }
    }

    void loadPage() {
        if (m_tab == 2) { render(); return; }
        if (m_inFlight) { renderRoadmap(); return; }
        std::string ids;
        std::vector<int> missing;
        const auto begin = view().page * pageSize;
        const auto end = std::min(m_results.size(), begin + pageSize);
        for (size_t i = begin; i < end; ++i) {
            const int id = m_roadmap[m_results[i]].id;
            if (!m_cache.contains(id)) {
                missing.push_back(id);
                if (!ids.empty()) ids += ',';
                ids += std::to_string(id);
            }
        }
        if (missing.empty()) { renderRoadmap(); return; }
        auto manager = GameLevelManager::get();
        auto search = GJSearchObject::create(SearchType::Type19);
        search->m_searchQuery = ids;
        m_requestKey = search->getKey();
        m_requestedIds = std::move(missing);
        m_requestGeneration = m_generation;
        m_networkMessage.clear();
        if (std::exchange(m_forceRefresh, false))
            manager->m_storedLevels->removeObjectForKey(m_requestKey.c_str());
        if (auto cached = manager->getStoredOnlineLevels(m_requestKey.c_str())) {
            cacheLevels(cached);
            m_requestedIds.clear();
            m_requestKey.clear();
            syncProgress();
            renderRoadmap();
            return;
        }
        m_inFlight = true;
        m_requestSearch = search;
        m_networkMessage.clear();
        scheduleOnce(schedule_selector(RoadmapListLayer::onRequestTimeout), 15.f);
        schedule(schedule_selector(RoadmapListLayer::onRequestRetry), .25f);
        renderRoadmap();
        onRequestRetry(0.f);
    }

    void cacheLevels(CCArray* levels) {
        if (!levels) return;
        for (auto level : CCArrayExt<GJGameLevel*>(levels)) {
            if (!level) continue;
            const int id = static_cast<int>(level->m_levelID);
            if (knownId(id)) m_cache[id] = level;
        }
    }

    void onTab(CCObject* sender) {
        if (m_tab != 2) view().query = lower(m_search->getString());
        m_tab = sender->getTag();
        ++m_generation;
        if (m_tab != 2) m_search->setString(view().query);
        syncProgress();
        rebuildResults();
        loadPage();
    }

    void onFilter(CCObject*) {
        if (m_tab == 2) return;
        view().filter = (view().filter + 1) % std::size(filters);
        ++m_generation;
        rebuildResults(true);
        loadPage();
    }

    void onSearch(CCObject*) {
        if (m_tab == 2) return;
        view().query = lower(m_search->getString());
        ++m_generation;
        rebuildResults(true);
        loadPage();
    }

    void onResetSearch(CCObject*) {
        if (m_tab == 2) return;
        view().query.clear();
        view().filter = 0;
        m_search->setString("");
        ++m_generation;
        rebuildResults(true);
        loadPage();
    }

    void onPrevious(CCObject*) {
        if (m_tab != 2 && view().page > 0) {
            --view().page;
            ++m_generation;
            loadPage();
        }
    }

    void onNext(CCObject*) {
        if (m_tab != 2 && (view().page + 1) * pageSize < m_results.size()) {
            ++view().page;
            ++m_generation;
            loadPage();
        }
    }

    void onRefresh(CCObject*) {
        ++m_generation;
        stopRequest();
        m_forceRefresh = m_tab != 2;
        if (m_tab != 2) {
            const auto begin = view().page * pageSize;
            const auto end = std::min(m_results.size(), begin + pageSize);
            for (size_t i = begin; i < end; ++i) m_cache.erase(m_roadmap[m_results[i]].id);
        }
        m_networkMessage.clear();
        syncProgress();
        rebuildResults();
        loadPage();
    }

    void onTools(CCObject*) {
        std::vector<int> ids;
        for (auto const& entry : m_roadmap) ids.push_back(entry.id);
        Ref<RoadmapListLayer> self(this);
        if (auto popup = roadui::ToolsPopup::create(std::move(ids), [self] {
            if (self->isRunning()) self->refreshLocal();
        })) popup->show();
    }
    void onStats(CCObject*) { roadui::StatsPopup::open(); }

    void onBack(CCObject*) {
        CCDirector::get()->popSceneWithTransition(.3f, kPopTransitionFade);
    }

public:
    static RoadmapListLayer* create() {
        auto layer = new RoadmapListLayer();
        if (layer->init()) { layer->autorelease(); return layer; }
        delete layer;
        return nullptr;
    }

    static CCScene* scene() {
        auto scene = CCScene::create();
        if (auto layer = RoadmapListLayer::create()) scene->addChild(layer);
        return scene;
    }

    bool init() override {
        using namespace roadui::layout;
        if (!CCLayer::init()) return false;
        setID("RoadmapListLayer"_spr);
        m_roadmap = getRoadmap();
        const auto screen = CCDirector::get()->getWinSize();
        roadui::rect(this, 0, 0, screen.width, screen.height, roadui::background);
        const float scale = scaleFor(screen.width, screen.height);
        m_window = roadui::panel(this, (screen.width - width * scale) / 2,
                                (screen.height - height * scale) / 2,
                                width, height);
        m_window->setScale(scale);
        m_window->setID("roadmap-window"_spr);
        roadui::rect(m_window, 1, 273, width - 2, 28, roadui::inset);
        roadui::label(m_window, "THE ROAD OF DOOM AND DESPAIR",
                      width / 2, 288, 306, .44f);
        auto header = roadui::menu(m_window);
        roadui::button(header, this, menu_selector(RoadmapListLayer::onBack),
                       "<", 31, 288, 26, 22);
        roadui::button(header, this, menu_selector(RoadmapListLayer::onTools),
                       "OUTILS", width - 42, 288, 64, 22);

        static constexpr char const* tabNames[] = {"PARCOURS", "STARTPOS", "PROGRESSION"};
        auto tabs = roadui::menu(m_window);
        for (int i = 0; i < 3; ++i) {
            const auto bounds = roadui::layout::tab(i);
            m_tabs[i] = roadui::button(tabs, this, menu_selector(RoadmapListLayer::onTab),
                                      tabNames[i], bounds.x + bounds.width / 2,
                                      bounds.y + bounds.height / 2,
                                      bounds.width, bounds.height, i == 0);
            m_tabs[i]->setTag(i);
        }

        m_toolbar = CCNode::create();
        m_toolbar->setPosition({toolbar.x, toolbar.y});
        m_window->addChild(m_toolbar);
        m_search = roadui::input(m_toolbar, 142, 12, 284, "Rechercher un niveau ou un ID...");
        m_search->setMaxCharCount(80);
        auto searchActions = roadui::menu(m_toolbar);
        roadui::button(searchActions, this, menu_selector(RoadmapListLayer::onSearch),
                       "OK", 305, 12, 32, 24);
        m_filter = roadui::button(searchActions, this, menu_selector(RoadmapListLayer::onFilter),
                                  "TOUS", 380, 12, 96, 24);
        m_dashboardHeading = roadui::label(m_window, "TON PARCOURS VERS NULLSCAPES",
                                          padding, 230, 306, .28f, roadui::muted, true);
        m_dashboardHeading->setVisible(false);
        m_statsButton = roadui::button(header, this, menu_selector(RoadmapListLayer::onStats),
                                      "STATS", 394, 230, 100, 24);
        m_statsButton->setVisible(false);

        m_content = CCNode::create();
        m_content->setPosition({padding, contentBottom});
        m_content->setContentSize({contentWidth, contentHeight});
        m_content->setID("roadmap-content"_spr);
        m_window->addChild(m_content);

        roadui::rect(m_window, padding, 34, contentWidth, 1, roadui::outline);
        auto footer = roadui::menu(m_window);
        m_previous = roadui::button(footer, this, menu_selector(RoadmapListLayer::onPrevious),
                                    "<", 153, 20, 28, 22);
        m_next = roadui::button(footer, this, menu_selector(RoadmapListLayer::onNext),
                                ">", 307, 20, 28, 22);
        roadui::button(footer, this, menu_selector(RoadmapListLayer::onRefresh),
                       "ACTUALISER", 402, 20, 82, 22);
        m_progress = roadui::label(m_window, "", 70, 20, 100, .28f, roadui::muted);
        m_pageLabel = roadui::label(m_window, "", width / 2, 20, 116, .27f);
        setKeypadEnabled(true);
        setKeyboardEnabled(true);
        syncProgress();
        rebuildResults();
        renderRoadmap();
        return true;
    }

    void onEnter() override {
        CCLayer::onEnter();
        syncProgress();
        rebuildResults();
        loadPage();
    }

    void onExit() override {
        stopRequest();
        CCLayer::onExit();
    }

    void loadLevelsFinished(CCArray* levels, const char* key, int) override {
        if (!m_inFlight || (key && m_requestKey != key)) return;
        const bool stale = m_requestGeneration != m_generation;
        cacheLevels(levels);
        const auto requested = m_requestedIds.size();
        stopRequest();
        syncProgress();
        if (Mod::get()->getSettingValue<bool>("diagnostic-logging"))
            log::info("[RDoD][Fetch] requested={} returned={} stale={} tab={}",
                      requested, levels ? levels->count() : 0, stale, m_tab);
        // Always render the current query/page from cache. Never let an older
        // response replace a different page or the progression dashboard.
        rebuildResults();
        render();
        if (stale && m_tab != 2) loadPage();
    }

    void loadLevelsFailed(const char* key, int) override {
        if (!m_inFlight || (key && m_requestKey != key)) return;
        const bool stale = m_requestGeneration != m_generation;
        stopRequest();
        m_networkMessage = "Echec du chargement. Actualise pour reessayer.";
        render();
        if (stale && m_tab != 2) loadPage();
    }

    void setupPageInfo(gd::string, const char*) override { updateChrome(); }

    void keyDown(enumKeyCodes key, double time) override {
        if (key == KEY_Left || key == CONTROLLER_Left) onPrevious(nullptr);
        else if (key == KEY_Right || key == CONTROLLER_Right) onNext(nullptr);
        else if (key == KEY_Enter) onSearch(nullptr);
        else CCLayer::keyDown(key, time);
    }

    void keyBackClicked() override { onBack(nullptr); }

};

// The only injected entry point remains the search-filter menu.
class $modify(RoadmapSearch, LevelSearchLayer) {
    bool init(int type) {
        if (!LevelSearchLayer::init(type)) return false;
        auto sprite = CCSprite::createWithSpriteFrameName("GJ_starsIcon_001.png");
        CCNode* image = nullptr;
        if (sprite) {
            auto circle = CircleButtonSprite::create(sprite);
            circle->setScale(.8f);
            image = circle;
        } else {
            image = CCLabelBMFont::create("PARCOURS", "bigFont.fnt");
            image->setScale(.35f);
        }
        auto button = CCMenuItemSpriteExtra::create(image, this,
                             menu_selector(RoadmapSearch::openRoadmap));
        button->setID("roadmap-button"_spr);
        if (auto menu = getChildByID("other-filter-menu")) {
            menu->addChild(button);
            menu->updateLayout();
        } else {
            auto fallbackMenu = roadui::menu(this);
            const auto screen = CCDirector::get()->getWinSize();
            button->setPosition({screen.width - 28.f, screen.height * .35f});
            fallbackMenu->addChild(button);
        }
        return true;
    }

    void openRoadmap(CCObject*) {
        CCDirector::get()->pushScene(
            CCTransitionFade::create(.3f, RoadmapListLayer::scene()));
    }
};

