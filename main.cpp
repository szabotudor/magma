#include "engine.hpp"


int main(int argc, char** argv) {
    mgm::MagmaEngine engine{{argv + 1, argv + argc}};
    engine.run();
    return 0;
}
