#include <Geode/Geode.hpp>
#include <Geode/modify/LevelSearchLayer.hpp>
#include <Geode/ui/BasedButtonSprite.hpp>
#include <Geode/ui/TextInput.hpp>
#include <Geode/ui/Popup.hpp>
#include "level_data.hpp"
#include <algorithm>
#include <cctype>
#include <string>
#include <vector>
using namespace geode::prelude;

namespace {
struct Entry { std::string name, id; };
std::vector<Entry> parse() {
    std::vector<Entry> out;
    std::string data=road::levels;
    size_t pos=0;
    while(pos<data.size()){
        auto end=data.find('\n',pos);
        auto s=data.substr(pos,end==std::string::npos?std::string::npos:end-pos);
        auto sep=s.find('|');
        if(sep!=std::string::npos) out.push_back({s.substr(0,sep),s.substr(sep+1)});
        if(end==std::string::npos) break;
        pos=end+1;
    }
    return out;
}
std::string lower(std::string s) {
    for(auto& c:s) c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}
void rect(CCNode* parent,float x,float y,float w,float h,ccColor4B color) {
    auto n=CCLayerColor::create(color,w,h);
    n->setPosition({x,y});
    parent->addChild(n);
}
CCLabelBMFont* label(CCNode* parent,std::string const& s,float x,float y,
    float scale,ccColor3B color={255,255,255},float width=0.f,
    CCPoint anchor={0.f,.5f}) {
    auto n=CCLabelBMFont::create(s.c_str(),"bigFont.fnt");
    n->setAnchorPoint(anchor);
    n->setPosition({x,y});
    n->setScale(scale);
    n->setColor(color);
    if(width>0.f)n->limitLabelWidth(width,scale,.15f);
    parent->addChild(n);
    return n;
}
}
class RoadmapPopup : public geode::Popup {
    std::vector<Entry> all;
    std::vector<size_t> matches;
    size_t page=0;
    CCNode* cards=nullptr;
    CCLabelBMFont* pageLabel=nullptr;
    TextInput* searchBox=nullptr;
    void button(char const* title,CCPoint p,SEL_MenuHandler callback) {
        auto spr=ButtonSprite::create(title);
        spr->setScale(.48f);
        auto item=CCMenuItemSpriteExtra::create(spr,this,callback);
        item->setPosition(p);
        m_buttonMenu->addChild(item);
    }
    void card(size_t idx,float y) {
        auto const& level=all[idx];
        rect(cards,22.f,y-44.f,376.f,89.f,{57,27,11,255});
        rect(cards,25.f,y-41.f,370.f,83.f,{135,77,38,255});
        rect(cards,29.f,y-37.f,362.f,75.f,{99,54,27,255});
        rect(cards,32.f,y-33.f,58.f,67.f,{24,49,53,255});
        rect(cards,36.f,y-29.f,50.f,59.f,{31,72,74,255});
        label(cards,fmt::format("#{}",idx+1),61.f,y+11.f,.55f,{255,221,79},50.f,{.5f,.5f});
        label(cards,"DEMON",61.f,y-22.f,.27f,{244,239,175},52.f,{.5f,.5f});
        label(cards,level.name,99.f,y+22.f,.59f,{255,255,255},184.f);
        auto status=idx==0?"COMPLETED":idx==1?"IN PROGRESS":"TO BEAT";
        auto color=idx==0?ccColor3B{130,255,148}:idx==1?ccColor3B{255,228,111}:ccColor3B{219,214,205};
        label(cards,status,100.f,y+2.f,.36f,color,180.f);
        label(cards,idx==0?"ATTEMPTS: 1,3K":"ATTEMPTS: --",
            100.f,y-15.f,.28f,{249,220,158},180.f);
        label(cards,idx==0?"BEST: 100%":"BEST: --",
            100.f,y-29.f,.28f,{238,236,212},180.f);
        auto view=ButtonSprite::create("VIEW");
        view->setScale(.61f);
        auto item=CCMenuItemSpriteExtra::create(view,this,menu_selector(RoadmapPopup::openLevel));
        item->setPosition({335.f,y+7.f});
        item->setTag(static_cast<int>(idx));
        auto menu=CCMenu::create();
        menu->setPosition({0.f,0.f});
        menu->addChild(item);
        cards->addChild(menu,1);
        label(cards,fmt::format("ID {}",level.id),335.f,y-27.f,.27f,
              {200,224,224},105.f,{.5f,.5f});
    }
    void render() {
        cards->removeAllChildrenWithCleanup(true);
        page=matches.empty()?0:std::min(page,(matches.size()-1)/2);
        pageLabel->setString(fmt::format("{} - {} OF {}  |  PAGE {} / {}",
            matches.empty()?0:page*2+1,
            std::min(matches.size(),page*2+2),matches.size(),
            page+1,std::max<size_t>(1,(matches.size()+1)/2)).c_str());
        if(matches.empty()){
            label(cards,"NO LEVEL FOUND",210.f,147.f,.55f,{255,235,154},310.f,{.5f,.5f});
            return;
        }
        for(size_t i=page*2;i<std::min(matches.size(),page*2+2);++i)
            card(matches[i],i%2==0?185.f:91.f);
    }
    void openLevel(CCObject* sender) {
        auto idx=static_cast<CCNode*>(sender)->getTag();
        if(idx<0||static_cast<size_t>(idx)>=all.size())return;
        CCDirector::sharedDirector()->pushScene(CCTransitionFade::create(.25f,
            LevelBrowserLayer::scene(GJSearchObject::create(SearchType::Search,all[idx].id))));
    }
    void previous(CCObject*) {if(page)--page;render();}
    void next(CCObject*) {if((page+1)*2<matches.size())++page;render();}
    void search(CCObject*) {
        auto q=lower(searchBox->getString());
        matches.clear();
        for(size_t i=0;i<all.size();++i)
            if(lower(all[i].name).find(q)!=std::string::npos||
               all[i].id.find(q)!=std::string::npos) matches.push_back(i);
        page=0;render();
    }
    bool setup() {
        if(!Popup::init(420.f,310.f))return false;
        all=parse();
        for(size_t i=0;i<all.size();++i)matches.push_back(i);
        rect(m_mainLayer,10.f,40.f,11.f,230.f,{32,165,24,255});
        rect(m_mainLayer,399.f,40.f,10.f,230.f,{32,165,24,255});
        rect(m_mainLayer,20.f,274.f,380.f,34.f,{16,115,45,255});
        rect(m_mainLayer,24.f,277.f,372.f,27.f,{28,163,83,255});
        label(m_mainLayer,"ROAD OF DOOM & DESPAIR",210.f,295.f,.54f,
            {255,255,255},341.f,{.5f,.5f});
        label(m_mainLayer,"1 / 75 COMPLETED",210.f,279.f,.29f,
            {229,251,147},300.f,{.5f,.5f});
        rect(m_mainLayer,22.f,239.f,376.f,31.f,{98,48,20,255});
        searchBox=TextInput::create(244.f,"SEARCH LEVELS OR IDS...");
        searchBox->setPosition({172.f,253.f});
        m_mainLayer->addChild(searchBox);
        button("GO",{366.f,253.f},menu_selector(RoadmapPopup::search));
        cards=CCNode::create();
        cards->setPosition({0.f,0.f});
        m_mainLayer->addChild(cards);
        button("<",{37.f,18.f},menu_selector(RoadmapPopup::previous));
        button(">",{383.f,18.f},menu_selector(RoadmapPopup::next));
        pageLabel=label(m_mainLayer,"",210.f,19.f,.31f,
            {255,255,255},310.f,{.5f,.5f});
        render();
        return true;
    }
public:
    static RoadmapPopup* create() {
        auto p=new RoadmapPopup;
        if(p->setup()){p->autorelease();return p;}
        delete p;return nullptr;
    }
};
class $modify(RoadmapSearch,LevelSearchLayer) {
    bool init(int type) {
        if(!LevelSearchLayer::init(type))return false;
        auto spr=ButtonSprite::create("RD");
        spr->setScale(.60f);
        auto item=CCMenuItemSpriteExtra::create(spr,this,menu_selector(RoadmapSearch::openRoadmap));
        item->setID("roadmap-button"_spr);
        if(auto menu=getChildByID("other-filter-menu")){
            menu->addChild(item);menu->updateLayout();
        } else {
            auto fallback=CCMenu::create();
            auto size=CCDirector::sharedDirector()->getWinSize();
            fallback->setPosition({size.width-28.f,size.height*.35f});
            fallback->addChild(item);
            addChild(fallback,50);
        }
        return true;
    }
    void openRoadmap(CCObject*){if(auto p=RoadmapPopup::create())p->show();}
};
