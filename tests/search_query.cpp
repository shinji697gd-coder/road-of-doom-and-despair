#include "../src/SearchQuery.hpp"
#include <cassert>

int main() {
    assert(road::startposQuery("Acu") == "Acu startpos");
    assert(road::startposQuery("Forest Temple") == "Forest Temple startp");
    assert(road::startposQuery("Sonic Wave Infinity") == "Sonic Wave Infinity");
}
