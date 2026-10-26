#pragma once
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <iomanip>
#include <cmath>
namespace training {
struct Preset { std::string name; double percent=0; };
struct Progress {
 int best=0; bool favorite=false; int status=0;
 std::string notes; std::vector<std::pair<double,double>> runs;
 std::vector<Preset> presets;
};
struct Store {
 std::map<int,Progress> levels; int goal=0;
 std::string encode() const {
  std::ostringstream s; s << "RDOD 1 " << goal << ' ' << levels.size() << '\n';
  for(auto const& [id,p]:levels) {
   s<<id<<' '<<p.best<<' '<<p.favorite<<' '<<p.status<<' '<<std::quoted(p.notes)<<' '<<p.runs.size()<<' '<<p.presets.size()<<'\n';
   for(auto [a,b]:p.runs) s<<a<<' '<<b<<'\n';
   for(auto const& q:p.presets) s<<std::quoted(q.name)<<' '<<q.percent<<'\n';
  } return s.str();
 }
 bool decode(std::string const& data) {
  if(data.size()>2000000) return false;
  std::istringstream s(data); Store next; std::string magic; int schema,n;
  if(!(s>>magic>>schema>>next.goal>>n)||magic!="RDOD"||schema!=1||n<0||n>75) return false;
  for(int i=0;i<n;i++) {
   int id,r,q; Progress p;
   if(!(s>>id>>p.best>>p.favorite>>p.status>>std::quoted(p.notes)>>r>>q)||id<=0||p.best<0||p.best>100||p.status<0||p.status>3||r<0||r>1000||q<0||q>1000||p.notes.size()>4096||next.levels.contains(id)) return false;
   for(int j=0;j<r;j++){double a,b;if(!(s>>a>>b)||!std::isfinite(a)||!std::isfinite(b)||a<0||a>=b||b>100)return false;p.runs.emplace_back(a,b);}
   for(int j=0;j<q;j++){Preset v;if(!(s>>std::quoted(v.name)>>v.percent)||v.name.size()>80||!std::isfinite(v.percent)||v.percent<0||v.percent>=100)return false;p.presets.push_back(v);}
   next.levels[id]=p;
  }
  s>>std::ws; if(!s.eof()) return false; *this=std::move(next); return true;
 }
 bool writable=true;
 void save();
 static Store& get();
};
}
