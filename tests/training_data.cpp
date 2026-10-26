#include "../src/TrainingData.hpp"
#include <cassert>
int main(){
 training::Store s; s.goal=109780665;auto& p=s.levels[s.goal];p.best=72;p.favorite=true;p.notes="quote \" and newline\n";p.runs={{42,100}};p.presets={{"Ending",72.8}};
 training::Store t;assert(t.decode(s.encode()));assert(t.encode()==s.encode());
 auto original=t.encode();assert(!t.decode("RDOD 2 0 0"));assert(t.encode()==original);
 assert(!t.decode("RDOD 1 0 1\n1 101 0 0 \"\" 0 0"));
 assert(!t.decode("RDOD 1 0 1\n1 10 0 0 \"\" 1 0\n80 50"));
 assert(!t.decode("RDOD 1 0 0 garbage"));
 assert(!t.decode("RDOD 1 0 2\n1 0 0 0 \"\" 0 0\n1 0 0 0 \"\" 0 0"));
 assert(t.encode()==original);
}
