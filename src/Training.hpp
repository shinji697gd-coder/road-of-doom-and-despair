#pragma once
#include <Geode/Geode.hpp>
#include "TrainingData.hpp"
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <map>
#include <cmath>
using namespace geode::prelude;
namespace training {
inline void Store::save(){if(writable)Mod::get()->setSavedValue("training-v1",encode());}
inline Store& Store::get(){static Store s;static bool loaded=false;if(!loaded){loaded=true;auto raw=Mod::get()->getSavedValue<std::string>("training-v1","");if(!raw.empty()&&!s.decode(raw)){s.writable=false;log::error("[RDoD] Invalid training save: writing disabled, original retained");}}return s;}
class Detail final : public Popup {
 size_t selected=0;
 CCLabelBMFont* statusLabel=nullptr;
 int id; TextInput *notes,*label,*from,*to; CCLabelBMFont* summary;
 void refresh(){auto& s=Store::get();auto& p=s.levels[id];const char* statuses[]={"NOT STARTED","LEARNING","GRINDING","PAUSED"};if(statusLabel)statusLabel->setString(statuses[p.status]);summary->setString(fmt::format("{}% | {} | {}\n{} runs | {} presets (PARTIAL)",p.best,p.favorite?"FAVORITE":"-",s.goal==id?"GOAL":"-",p.runs.size(),p.presets.size()).c_str());}
 void save(CCObject*){Store::get().levels[id].notes=notes->getString();Store::get().save();refresh();}
 void status(CCObject*){auto& p=Store::get().levels[id];p.status=(p.status+1)%4;Store::get().save();refresh();}
 void favorite(CCObject*){auto& p=Store::get().levels[id];p.favorite=!p.favorite;Store::get().save();refresh();}
 void goal(CCObject*){auto& s=Store::get();s.goal=s.goal==id?0:id;s.save();refresh();}
 bool number(TextInput* input,double& n){auto str=input->getString();std::istringstream s(str);if(!(s>>n))return false;s>>std::ws;return s.eof()&&std::isfinite(n);}
 void run(CCObject*){double a,b;if(!number(from,a)||!number(to,b)||a<0||a>=b||b>100){FLAlertLayer::create("Run","Enter 0 <= FROM < TO <= 100.","OK")->show();return;}auto& p=Store::get().levels[id];if(p.runs.size()<1000)p.runs.emplace_back(a,b);Store::get().save();refresh();}
 void preset(CCObject*){double a;if(!number(from,a)||a<0||a>=100||label->getString().empty()){FLAlertLayer::create("Startpos","Enter a name and FROM percentage (0 to 99.99).","OK")->show();return;}auto& p=Store::get().levels[id];if(p.presets.size()<1000)p.presets.push_back({label->getString(),a});Store::get().save();refresh();}
 void list(CCObject*){auto& p=Store::get().levels[id];std::string text="Saved training records:\n";size_t shown=0;for(auto [a,b]:p.runs)if(shown++<8)text+=fmt::format("Run {}-{}%\n",a,b);shown=0;for(auto const& q:p.presets)if(shown++<8)text+=fmt::format("{}: {}% (PARTIAL)\n",q.name,q.percent);FLAlertLayer::create("Records",text.c_str(),"OK")->show();}
 void remove(CCObject*){auto& p=Store::get().levels[id];if(selected<p.presets.size())p.presets.erase(p.presets.begin()+selected);selected=0;Store::get().save();refresh();}
 void next(CCObject*){auto& v=Store::get().levels[id].presets;if(v.empty())return;selected=(selected+1)%v.size();label->setString(v[selected].name);from->setString(fmt::format("{}",v[selected].percent));refresh();}
 void edit(CCObject*){auto& v=Store::get().levels[id].presets;double a;if(selected>=v.size()||!number(from,a)||a<0||a>=100||label->getString().empty())return;v[selected]={label->getString(),a};Store::get().save();refresh();}
 void apply(CCObject*){FLAlertLayer::create("Startpos TEST","APPLY is locked: these are percentage markers, not validated native start positions. In-game isolation testing is required.","OK")->show();}
 void button(const char* text,float x,float y,SEL_MenuHandler cb){auto l=CCLabelBMFont::create(text,"goldFont.fnt");l->setScale(.45f);auto b=CCMenuItemSpriteExtra::create(l,this,cb);b->setPosition({x,y});m_buttonMenu->addChild(b);}
 TextInput* input(float w,const char* hint,float x,float y){auto v=TextInput::create(w,hint);v->setPosition({x,y});m_mainLayer->addChild(v);return v;}
 bool setup(int level,std::string const& name){if(!Popup::init(400,280))return false;id=level;setTitle(name);auto& p=Store::get().levels[id];summary=CCLabelBMFont::create("","bigFont.fnt");summary->setScale(.4f);summary->setPosition({200,228});m_mainLayer->addChild(summary);
 statusLabel=CCLabelBMFont::create("", "goldFont.fnt");statusLabel->setScale(.35f);auto statusButton=CCMenuItemSpriteExtra::create(statusLabel,this,menu_selector(Detail::status));statusButton->setPosition({200,204});m_buttonMenu->addChild(statusButton);
 notes=input(340,"Notes",200,180);notes->setString(p.notes);notes->setMaxCharCount(4096);
 label=input(180,"Preset name",115,137);label->setMaxCharCount(80);from=input(65,"From %",250,137);to=input(65,"To %",330,137);
 button("SAVE NOTES",75,101,menu_selector(Detail::save));button("FAVORITE",205,101,menu_selector(Detail::favorite));button("GOAL",325,101,menu_selector(Detail::goal));
 button("ADD RUN",75,72,menu_selector(Detail::run));button("ADD PRESET",210,72,menu_selector(Detail::preset));button("RECORDS",330,72,menu_selector(Detail::list));
 button("DELETE PRESET",65,40,menu_selector(Detail::remove));button("APPLY",345,40,menu_selector(Detail::apply));button("NEXT",175,40,menu_selector(Detail::next));button("EDIT",255,40,menu_selector(Detail::edit));refresh();return true;}
public: static Detail* create(int id,std::string const& name){auto p=new Detail;if(p->setup(id,name)){p->autorelease();return p;}delete p;return nullptr;}
};
}
