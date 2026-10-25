#include <Geode/Geode.hpp>
#include <Geode/modify/LevelSearchLayer.hpp>
#include <Geode/binding/AppDelegate.hpp>
#include <Geode/ui/ScrollLayer.hpp>
#include <Geode/binding/LevelCell.hpp>
#include <Geode/binding/GameLevelManager.hpp>
#include <Geode/binding/GJListLayer.hpp>
#include <Geode/binding/GJSearchObject.hpp>
#include <Geode/binding/LoadingCircle.hpp>
#include <Geode/ui/BasedButtonSprite.hpp>
#include <Geode/ui/TextInput.hpp>
#include "level_data.hpp"
#include "Training.hpp"
#include <Geode/ui/GeodeUI.hpp>
#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

using namespace geode::prelude;

// Layout and search approach adapted from Integrated Demonlist by hiimjasmine00 (MIT).
// Unlike the first mockup, this displays genuine Geometry Dash LevelCells, so
// the game's own VIEW buttons open the actual level information page.
namespace {
    struct Entry { std::string name; std::string id; };

    std::vector<Entry> getRoadmap() {
        std::vector<Entry> rows;
        std::string source = road::levels;
        size_t start = 0;
        while (start < source.size()) {
            auto end = source.find('\n', start);
            auto line = source.substr(start, end == std::string::npos ?
                                      std::string::npos : end - start);
            if (!line.empty() && line.back() == '\r') line.pop_back();
            auto sep = line.find('|');
            if (sep != std::string::npos && sep + 1 < line.size())
                rows.push_back({ line.substr(0, sep), line.substr(sep + 1) });
            if (end == std::string::npos) break;
            start = end + 1;
        }
        return rows;
    }

    std::string lower(std::string word) {
        for (auto& c : word)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return word;
    }
}

class RoadmapListLayer final : public CCLayer, public LevelManagerDelegate {
    std::vector<Entry> m_roadmap;
    std::vector<size_t> m_results;
    std::vector<bool> m_completed;
    unsigned m_filter = 0; // 0: all, 1: remaining, 2: completed
    GJListLayer* m_list = nullptr;
    ScrollLayer* m_scroll = nullptr;
    LoadingCircle* m_loading = nullptr;
    CCMenu* m_searchMenu = nullptr;
    TextInput* m_searchBox = nullptr;
    CCLabelBMFont* m_countLabel = nullptr;
    CCLabelBMFont* m_pageLabel = nullptr;
    CCLabelBMFont* m_progressLabel = nullptr;
    CCLabelBMFont* m_filterLabel = nullptr;
    CCMenuItemSpriteExtra* m_leftButton = nullptr;
    CCMenuItemSpriteExtra* m_rightButton = nullptr;
    CCMenuItemSpriteExtra* m_firstButton = nullptr;
    CCMenuItemSpriteExtra* m_lastButton = nullptr;
    CCMenuItemSpriteExtra* m_pageButton = nullptr;
    bool m_inFlight=false, m_reloadPending=false;
    unsigned m_tab=0;
    unsigned m_page = 0;
    std::string m_query;
    static constexpr size_t kPageSize = 10;

    CCMenuItemSpriteExtra* addMenuButton(CCMenu* menu, const char* sprite,
                                         CCPoint pos, SEL_MenuHandler callback,
                                         const char* name) {
        auto image = CCSprite::createWithSpriteFrameName(sprite);
        auto item = CCMenuItemSpriteExtra::create(image, this, callback);
        item->setPosition(pos);
        item->setID(name);
        menu->addChild(item);
        return item;
    }

    void updateProgressLabel() {
        if (!m_progressLabel) return;
        const auto done = std::count(m_completed.begin(), m_completed.end(), true);
        m_progressLabel->setString(
            fmt::format("PROGRESS: {}/{}", done, m_roadmap.size()).c_str());
    }

    void rebuildResults() {
        m_results.clear();
        for (size_t i = 0; i < m_roadmap.size(); ++i) {
            auto& state=training::Store::get().levels[std::stoi(m_roadmap[i].id)];
            m_completed[i]=state.best==100;
            if(m_filter==3 && !state.favorite)continue;
            if(m_filter==4 && training::Store::get().goal!=std::stoi(m_roadmap[i].id))continue;
            if(m_filter==5 && state.presets.empty())continue;
            if (m_filter == 1 && m_completed[i]) continue;
            if (m_filter == 2 && !m_completed[i]) continue;
            if (lower(m_roadmap[i].name).find(m_query) != std::string::npos ||
                m_roadmap[i].id.find(m_query) != std::string::npos)
                m_results.push_back(i);
        }
        m_page = 0;
    }

    void onFilter(CCObject*) {
        m_filter = (m_filter + 1) % 6;
        static constexpr const char* labels[] = {
            "FILTER: ALL", "FILTER: TO DO", "FILTER: DONE", "FILTER: FAVORITES", "FILTER: GOAL", "FILTER: STARTPOS"
        };
        m_filterLabel->setString(labels[m_filter]);
        rebuildResults();
        loadPage();
    }

    void onToggleCompleted(CCObject* sender) {
        auto index=static_cast<size_t>(sender->getTag());
        if(index<m_roadmap.size()) training::Detail::create(std::stoi(m_roadmap[index].id),m_roadmap[index].name)->show();
    }
    void onDashboard(CCObject*) {
        m_tab=2;
        if(m_scroll){m_scroll->removeFromParentAndCleanup(true);m_scroll=nullptr;}
        auto& s=training::Store::get();int done=0,favorites=0,runs=0,presets=0;
        std::string goal="None";
        for(auto const& e:m_roadmap){auto& p=s.levels[std::stoi(e.id)];done+=p.best==100;favorites+=p.favorite;runs+=p.runs.size();presets+=p.presets.size();if(std::stoi(e.id)==s.goal)goal=e.name;}
        m_scroll=ScrollLayer::create({356,190});m_scroll->m_contentLayer->setContentSize({356,190});m_list->addChild(m_scroll,6);
        auto text=CCLabelBMFont::create(fmt::format("YOUR ROAD\n\nCompleted: {}/75 ({}%)\nGoal: {}\nFavorites: {}\nRuns: {} | Startpos: {}",done,done*100/75,goal,favorites,runs,presets).c_str(),"bigFont.fnt");text->setScale(.45f);text->limitLabelWidth(330,.45f,.2f);text->setPosition({178,110});m_scroll->m_contentLayer->addChild(text);
        auto bar=CCLayerColor::create({50,40,35,255},320,12);bar->setPosition({18,20});m_scroll->m_contentLayer->addChild(bar);auto fill=CCLayerColor::create({80,200,100,255},320.f*done/75,12);bar->addChild(fill);
        m_loading->setVisible(false);m_searchMenu->setVisible(false);updatePageControls();
    }
    void onRoadmap(CCObject*) {m_tab=0;m_filter=0;m_filterLabel->setString("FILTER: ALL");m_searchMenu->setVisible(true);onRefresh(nullptr);}
    void onSettings(CCObject*) {openSettingsPopup(Mod::get());}
    void onStartpos(CCObject*) {
        m_tab=1;m_searchMenu->setVisible(true);m_filter=5; m_filterLabel->setString("FILTER: STARTPOS");rebuildResults();loadPage();
    }
    void onBackup(CCObject*) {
        auto path=Mod::get()->getSaveDir()/"training-backup.json";
        std::ofstream f(path,std::ios::binary); matjson::Value backup;backup["schema"]=1;backup["payload"]=training::Store::get().encode();f<<backup.dump(); f.close();
        FLAlertLayer::create("Backup",f.fail()?"Backup failed.":"Saved training-backup.json in the mod save directory.","OK")->show();
    }
    void onRestore(CCObject*) {
        auto path=Mod::get()->getSaveDir()/"training-backup.json";std::ifstream f(path,std::ios::binary);
        if(!f || std::filesystem::file_size(path)>2000000){FLAlertLayer::create("Import","Backup missing or too large.","OK")->show();return;}
        std::string raw((std::istreambuf_iterator<char>(f)),{});training::Store candidate;
        auto json=matjson::parse(raw);
        if(!json || json.unwrap()["schema"].asInt().unwrapOr(0)!=1){FLAlertLayer::create("Import","Unsupported backup schema.","OK")->show();return;}
        raw=json.unwrap()["payload"].asString().unwrapOr("");
        if(!candidate.decode(raw)){FLAlertLayer::create("Import","Invalid backup. Nothing changed.","OK")->show();return;}
        for(auto const& [id,p]:candidate.levels){bool found=false;for(auto const& e:m_roadmap)if(std::stoi(e.id)==id)found=true;if(!found)return;}
        if(candidate.goal!=0){bool found=false;for(auto const& e:m_roadmap)if(std::stoi(e.id)==candidate.goal)found=true;if(!found)return;}
        auto& current=training::Store::get();
        std::ofstream before(Mod::get()->getSaveDir()/"training-before-import.txt");before<<current.encode();before.close();if(before.fail())return;
        for(auto const& [id,p]:current.levels) candidate.levels[id].best=std::max(candidate.levels[id].best,p.best);
        current=std::move(candidate);current.save();onRefresh(nullptr);
    }
    void addProgressButton(CCNode* row, size_t index) {
        auto menu = CCMenu::create();
        menu->setPosition({0.f, 0.f});
        menu->setID("progress-menu"_spr);
        auto label = CCLabelBMFont::create(
            "DETAILS", "goldFont.fnt");
        label->setScale(.52f);
        auto button = CCMenuItemSpriteExtra::create(
            label, this, menu_selector(RoadmapListLayer::onToggleCompleted));
        button->setTag(static_cast<int>(index));
        button->setPosition({310.f, 76.f});
        button->setID("progress-toggle"_spr);
        menu->addChild(button);
        row->addChild(menu, 20);
    }

    void updatePageControls() {
        size_t total = m_results.size();
        size_t pages = std::max<size_t>(1, (total + kPageSize - 1) / kPageSize);
        auto count = fmt::format("{} TO {} OF {}", total ? m_page * kPageSize + 1 : 0,
            std::min(total, (m_page + 1) * kPageSize), total);
        m_countLabel->setString(count.c_str());
        m_countLabel->limitLabelWidth(110.f, .6f, .0f);
        m_pageLabel->setString(fmt::format("{}", m_page + 1).c_str());
        bool multiple = pages > 1 && m_tab!=2;
        m_leftButton->setVisible(multiple && m_page > 0);
        m_firstButton->setVisible(multiple && m_page > 0);
        m_rightButton->setVisible(multiple && m_page + 1 < pages);
        m_lastButton->setVisible(multiple && m_page + 1 < pages);
        m_pageButton->setVisible(multiple);
    }

    void onPrevious(CCObject*) {
        if (m_page > 0) { --m_page; loadPage(); }
    }
    void onNext(CCObject*) {
        if ((m_page + 1) * kPageSize < m_results.size()) { ++m_page; loadPage(); }
    }
    void onFirst(CCObject*) { m_page = 0; loadPage(); }
    void onLast(CCObject*) {
        if (m_results.empty()) return;
        m_page = static_cast<unsigned>((m_results.size() - 1) / kPageSize);
        loadPage();
    }
    void onBack(CCObject*) {
        CCDirector::get()->popSceneWithTransition(.5f, kPopTransitionFade);
    }
    void onSearch(CCObject*) {
        std::string query = lower(m_searchBox->getString());
        m_query = query;
        rebuildResults();
        loadPage();
    }
    void onRefresh(CCObject*) { rebuildResults();updateProgressLabel();loadPage(); }

    void showEmptyPage() {
        renderRows(nullptr,true);
    }

    void loadPage() {
        if(m_tab==2){onDashboard(nullptr);return;}
        if(m_inFlight){m_reloadPending=true;renderRows(nullptr,false);return;}
        if (auto old = m_list->getChildByID("empty-label"_spr))
            old->removeFromParent();
        updatePageControls();
        if (m_results.empty()) { showEmptyPage(); return; }
        renderRows(nullptr,false);

        // Type19 requests a comma-separated list of *real* level IDs.
        // The game downloads level information, creator, song, difficulty,
        // stars, and all normal LevelCell controls by itself.
        auto search = GJSearchObject::create(SearchType::Type19);
        std::string ids;
        size_t start = m_page * kPageSize;
        size_t end = std::min(m_results.size(), start + kPageSize);
        for (size_t i = start; i < end; ++i) {
            if (!ids.empty()) ids += ',';
            ids += m_roadmap[m_results[i]].id;
        }
        search->m_searchQuery = ids;
        if(GameLevelManager::get()->m_levelManagerDelegate && GameLevelManager::get()->m_levelManagerDelegate!=this){renderRows(nullptr,true);return;}
        GameLevelManager::get()->m_levelManagerDelegate = this;
        m_inFlight=true;
        GameLevelManager::get()->getOnlineLevels(search);
    }

public:
    static RoadmapListLayer* create() {
        auto ret = new RoadmapListLayer();
        if (ret->init()) { ret->autorelease(); return ret; }
        delete ret;
        return nullptr;
    }

    static CCScene* scene() {
        auto scene = CCScene::create();
        scene->addChild(RoadmapListLayer::create());
        return scene;
    }

    bool init() override {
        if (!CCLayer::init()) return false;
        setID("RoadmapListLayer"_spr);
        m_roadmap = getRoadmap();
        for (const auto& entry : m_roadmap)
            m_completed.push_back(
                training::Store::get().levels[std::stoi(entry.id)].best==100);
        rebuildResults();

        auto screen = CCDirector::get()->getWinSize();
        auto background = CCSprite::create("GJ_gradientBG.png");
        background->setAnchorPoint({0.f, 0.f});
        background->setScaleX((screen.width + 10.f) / background->getContentWidth());
        background->setScaleY((screen.height + 10.f) / background->getContentHeight());
        background->setPosition({-5.f, -5.f});
        background->setColor({51,51,51});
        addChild(background);

        // Real native GJListLayer: green/cyan header and border, brown level panel.
        m_list = GJListLayer::create(nullptr, "The Road of Doom and Despair",
                       {0, 0, 0, 180}, 356.f, 220.f, 0);
        m_list->setPosition(screen / 2.f - m_list->getContentSize() / 2.f);
        m_list->setID("roadmap-list"_spr);
        addChild(m_list, 2);

        m_searchMenu = CCMenu::create();
        m_searchMenu->setContentSize({356.f,30.f});
        m_searchMenu->setPosition({0.f,190.f});
        m_searchMenu->setID("search-menu"_spr);
        m_list->addChild(m_searchMenu, 7);

        auto brown = CCLayerColor::create({194,114,62,255}, 356.f, 30.f);
        brown->setID("search-background"_spr);
        m_searchMenu->addChild(brown);

        m_searchBox = TextInput::create(310.f, "Search Levels...");
        m_searchBox->setTextAlign(TextInputAlign::Left);
        m_searchBox->setPosition({165.f,15.f});
        m_searchBox->getInputNode()->setLabelPlaceholderScale(.4f);
        m_searchBox->getInputNode()->setMaxLabelScale(.4f);
        m_searchBox->getBGSprite()->setContentSize({620.f,40.f});
        m_searchBox->getBGSprite()->setScale(.5f);
        m_searchBox->setID("search-input"_spr);
        m_searchMenu->addChild(m_searchBox);

        auto find = CCMenuItemSpriteExtra::create(
            CCSprite::createWithSpriteFrameName("gj_findBtn_001.png"),
            this, menu_selector(RoadmapListLayer::onSearch));
        find->setScale(.7f);
        find->setPosition({337.f,15.f});
        find->setID("find"_spr);
        m_searchMenu->addChild(find);

        auto controls = CCMenu::create();
        controls->setPosition({0.f,0.f});
        controls->setID("controls"_spr);
        addChild(controls, 5);
        m_leftButton = addMenuButton(controls,"GJ_arrow_03_001.png",
                       {24.f,screen.height/2.f},
                       menu_selector(RoadmapListLayer::onPrevious),"previous"_spr);
        m_rightButton = addMenuButton(controls,"GJ_arrow_03_001.png",
                        {screen.width-24.f,screen.height/2.f},
                        menu_selector(RoadmapListLayer::onNext),"next"_spr);
        static_cast<CCSprite*>(m_rightButton->getNormalImage())->setFlipX(true);
        addMenuButton(controls,"GJ_arrow_01_001.png",
                      {25.f,screen.height-25.f},
                      menu_selector(RoadmapListLayer::onBack),"back"_spr);

        m_firstButton = addMenuButton(controls,"GJ_arrow_02_001.png",
                        {22.f,screen.height/2.f-52.f},
                        menu_selector(RoadmapListLayer::onFirst),"first"_spr);
        m_firstButton->setScale(.65f);
        m_lastButton = addMenuButton(controls,"GJ_arrow_02_001.png",
                       {screen.width-22.f,screen.height/2.f-52.f},
                       menu_selector(RoadmapListLayer::onLast),"last"_spr);
        m_lastButton->setScale(.65f);
        static_cast<CCSprite*>(m_lastButton->getNormalImage())->setFlipX(true);

        addMenuButton(controls,"GJ_updateBtn_001.png",
                      {screen.width-22.f,22.f},
                      menu_selector(RoadmapListLayer::onRefresh),"refresh"_spr);

        m_countLabel = CCLabelBMFont::create("", "goldFont.fnt");
        m_countLabel->setAnchorPoint({1.f,1.f});
        m_countLabel->setPosition({screen.width-7.f,screen.height-3.f});
        m_countLabel->setScale(.6f);
        m_countLabel->setID("count"_spr);
        addChild(m_countLabel);

        m_progressLabel = CCLabelBMFont::create("", "goldFont.fnt");
        m_progressLabel->setPosition({screen.width / 2.f, screen.height - 14.f});
        m_progressLabel->setScale(.55f);
        m_progressLabel->setID("progress-count"_spr);
        addChild(m_progressLabel, 5);
        updateProgressLabel();

        m_filterLabel = CCLabelBMFont::create("FILTER: ALL", "goldFont.fnt");
        m_filterLabel->setScale(.62f);
        auto filterButton = CCMenuItemSpriteExtra::create(
            m_filterLabel, this, menu_selector(RoadmapListLayer::onFilter));
        filterButton->setPosition({screen.width / 2.f, 20.f});
        filterButton->setID("progress-filter"_spr);
        controls->addChild(filterButton);

        auto pageSprite = CCSprite::create("GJ_button_02.png");
        pageSprite->setScale(.7f);
        m_pageLabel = CCLabelBMFont::create("1","bigFont.fnt");
        m_pageLabel->setScale(.8f);
        m_pageLabel->setPosition(pageSprite->getContentSize()/2.f);
        pageSprite->addChild(m_pageLabel);
        m_pageButton = CCMenuItemSpriteExtra::create(pageSprite,this,
                                 menu_selector(RoadmapListLayer::onRefresh));
        m_pageButton->setPosition({screen.width-22.f,screen.height-40.f});
        m_pageButton->setID("page"_spr);
        controls->addChild(m_pageButton);

        m_loading = LoadingCircle::create();
        m_loading->setParentLayer(this);
        m_loading->setID("loading"_spr);
        m_loading->show();

        setKeypadEnabled(true);
        setKeyboardEnabled(true);
        auto tabs=CCMenu::create();tabs->setPosition({0,0});addChild(tabs,10);
        const char* names[]={"ROADMAP","STARTPOS","PROGRESSION","BACKUP","RESTORE","SETTINGS"};
        SEL_MenuHandler actions[]={menu_selector(RoadmapListLayer::onRoadmap),menu_selector(RoadmapListLayer::onStartpos),menu_selector(RoadmapListLayer::onDashboard),menu_selector(RoadmapListLayer::onBackup),menu_selector(RoadmapListLayer::onRestore),menu_selector(RoadmapListLayer::onSettings)};
        for(int i=0;i<6;i++){auto l=CCLabelBMFont::create(names[i],"goldFont.fnt");l->setScale(.35f);auto b=CCMenuItemSpriteExtra::create(l,this,actions[i]);b->setPosition(i<3?CCPoint{screen.width/2+(i-1)*115.f,40.f}:CCPoint{45.f,115.f-(i-3)*25.f});tabs->addChild(b);}
        loadPage();
        return true;
    }

    // Render all requested roadmap entries, even when the GD server does
    // not provide a level. This way the list is never an empty black panel.
    // Use explicitly-positioned LevelCells rather than a ColumnLayout, since
    // GenericContentLayer's visibility culling otherwise hides the rows.
    void renderRows(CCArray* levels, bool finished) {
        if(m_tab==2)return;
        if (m_scroll) {
            m_scroll->removeFromParentAndCleanup(true);
            m_scroll = nullptr;
        }

        constexpr float viewHeight = 190.f;
        constexpr float rowHeight = 90.f;
        constexpr float viewWidth = 356.f;
        const size_t begin = m_page * kPageSize;
        const size_t end = std::min(m_results.size(), begin+kPageSize);
        const size_t count = end - begin;
        const float contentHeight = std::max(viewHeight, count*rowHeight);

        m_scroll = ScrollLayer::create({viewWidth,viewHeight});
        m_scroll->setID("roadmap-scroll"_spr);
        m_scroll->setPosition({0.f,0.f});
        m_scroll->m_contentLayer->setContentSize({viewWidth,contentHeight});
        m_list->addChild(m_scroll,6);
        size_t actual = 0;

        for(size_t i=begin;i<end;i++) {
            const auto& wanted=m_roadmap[m_results[i]];
            const size_t row=i-begin;
            const float y=contentHeight - rowHeight*(row+1);

            GJGameLevel* level=nullptr;
            if (levels) {
                // The result array from getOnlineLevels contains GJGameLevel
                // objects. A dynamic typeinfo_cast could fail for GD's
                // non-RTTI classes and filter every result away.
                for (auto candidate : CCArrayExt<GJGameLevel*>(levels)) {
                    if(candidate && fmt::to_string(static_cast<int>(candidate->m_levelID)) == wanted.id) {
                        level=candidate;
                        break;
                    }
                }
            }
            if (level) {
                if(Mod::get()->getSettingValue<bool>("auto-sync-progress")) {
                    auto& p=training::Store::get().levels[std::stoi(wanted.id)];
                    p.best=std::max(p.best,std::clamp(level->getNormalPercent(),0,100));
                    m_completed[m_results[i]]=p.best==100;
                    training::Store::get().save();updateProgressLabel();
                }
                auto cell=LevelCell::create(viewWidth,rowHeight);
                if (!cell) continue;
                cell->setContentSize({viewWidth,rowHeight});
                cell->setAnchorPoint({0.f,0.f});
                cell->setPosition({0.f,y});
                cell->loadFromLevel(level);

                // Normal Geometry Dash level cells are usually created inside
                // a CustomListView/TableView. In our ScrollLayer, their menu
                // keeps an inappropriate offset: the creator name and VIEW
                // are drawn BELOW the level card, hidden by the next row.
                // Give the interactive elements explicit LOCAL coordinates.
                if (auto menu = cell->m_mainMenu) {
                    menu->setPosition({0.f,0.f});
                    if (auto creator = menu->getChildByID("creator-name")) {
                        creator->setAnchorPoint({0.f,.5f});
                        creator->setPosition({50.f,53.f});
                    }
                    if (auto view = menu->getChildByID("view-button")) {
                        view->setPosition({viewWidth-43.f,rowHeight*.5f});
                    }
                    // Level Thumbnails may add start-position buttons here.
                    // Keeping the native menu attached to each row lets
                    // these controls scroll with the respective level.
                }
                cell->updateBGColor(static_cast<int>(row));
                cell->setID(fmt::format("level-{}"_spr,wanted.id));
                m_scroll->m_contentLayer->addChild(cell,1);
                addProgressButton(cell, m_results[i]);
                actual++;
                continue;
            }

            // The row remains visible during loading or if an ID was not
            // returned by the Geometry Dash servers, and can be retried.
            auto bg=CCLayerColor::create(row%2==0?
                  ccColor4B{161,88,44,255}:ccColor4B{194,114,62,255},
                  viewWidth,rowHeight);
            bg->setPosition({0.f,y});
            bg->setID(fmt::format("placeholder-{}"_spr,wanted.id));
            m_scroll->m_contentLayer->addChild(bg);
            auto label=CCLabelBMFont::create(wanted.name.c_str(),"bigFont.fnt");
            label->setAnchorPoint({0.f,.5f});
            label->setScale(.57f);
            label->setPosition({19.f,58.f});
            label->limitLabelWidth(330.f,.57f,.25f);
            bg->addChild(label);
            auto idLabel=CCLabelBMFont::create(
                fmt::format("ID {}  |  {}",wanted.id,finished?"LEVEL NOT RECEIVED":"LOADING...").c_str(),
                "goldFont.fnt");
            idLabel->setAnchorPoint({0.f,.5f});
            idLabel->setScale(.38f);
            idLabel->setPosition({19.f,24.f});
            idLabel->limitLabelWidth(335.f,.38f,.2f);
            bg->addChild(idLabel);
            addProgressButton(bg, m_results[i]);
        }

        if (!count) {
            auto empty=CCLabelBMFont::create("NO LEVELS FOUND","bigFont.fnt");
            empty->setScale(.5f);
            empty->setPosition({178.f,95.f});
            m_scroll->m_contentLayer->addChild(empty);
        }
        m_scroll->scrollToTop();
        m_loading->setVisible(!finished && count>0);
        if(Mod::get()->getSettingValue<bool>("diagnostic-logging")) log::info("Roadmap page {}: requested={} server-returned={} matched={} finished={}",
            m_page+1,count,levels?levels->count():0,actual,finished);
        updatePageControls();
    }

    void loadLevelsFinished(CCArray* levels,const char*,int) override {
        auto manager=GameLevelManager::get();
        if(manager->m_levelManagerDelegate==this)
            manager->m_levelManagerDelegate=nullptr;
        m_inFlight=false;
        if(m_reloadPending){m_reloadPending=false;loadPage();return;}
        renderRows(levels,true);
    }

    void loadLevelsFailed(const char*,int) override {
        auto manager=GameLevelManager::get();
        if(manager->m_levelManagerDelegate==this)
            manager->m_levelManagerDelegate=nullptr;
        m_inFlight=false;
        if(m_reloadPending){m_reloadPending=false;loadPage();return;}
        renderRows(nullptr,true);
        FLAlertLayer::create("Loading issue",
            "The level information could not be fetched. The roadmap remains visible; use Refresh to retry.",
            "OK")->show();
    }

    void setupPageInfo(gd::string,const char*) override { updatePageControls(); }

    void keyDown(enumKeyCodes key,double time) override {
        if (key==KEY_Left || key==CONTROLLER_Left) onPrevious(nullptr);
        else if (key==KEY_Right || key==CONTROLLER_Right) onNext(nullptr);
        else if (key==KEY_Enter) onSearch(nullptr);
        else CCLayer::keyDown(key,time);
    }
    void keyBackClicked() override {onBack(nullptr);}

    void onExit() override {
        auto manager=GameLevelManager::get();
        if(manager->m_levelManagerDelegate==this)manager->m_levelManagerDelegate=nullptr;
        m_inFlight=false;m_reloadPending=false;
        CCLayer::onExit();
    }

    ~RoadmapListLayer() override {
        auto manager = GameLevelManager::get();
        if (manager->m_levelManagerDelegate == this)
            manager->m_levelManagerDelegate = nullptr;
    }
};

class $modify(RoadmapSearch, LevelSearchLayer) {
    bool init(int type) {
        if (!LevelSearchLayer::init(type)) return false;
        // Button lives alongside the other search filters just like Integrated Demonlist.
        auto image=CircleButtonSprite::createWithSpriteFrameName("GJ_starsIcon_001.png");
        image->getTopNode()->setScale(.75f);
        image->setScale(.8f);
        auto button=CCMenuItemSpriteExtra::create(image,this,
                                     menu_selector(RoadmapSearch::openRoadmap));
        button->setID("roadmap-button"_spr);
        if (auto menu=getChildByID("other-filter-menu")) {
            menu->addChild(button);
            menu->updateLayout();
        } else {
            auto fallback=CCMenu::create();
            auto screen=CCDirector::get()->getWinSize();
            fallback->setPosition({screen.width-28.f,screen.height*.35f});
            fallback->addChild(button);
            addChild(fallback, 50);
        }
        return true;
    }
    void openRoadmap(CCObject*) {
        CCDirector::get()->pushScene(
            CCTransitionFade::create(.5f,RoadmapListLayer::scene()));
    }
};

