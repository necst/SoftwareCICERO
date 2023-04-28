#include "Manager.h"

namespace Cicero {

Manager::Manager(Cicero::Instruction *program, int engineCount,
                 int windowSize) {
    for (int i = 0; i < engineCount; i++) {
        engines.push_back(Engine(program, windowSize));
    }
}

bool Manager::match(std::string input) {
    for (auto &engine : engines) {
        engine.reset(input);
    }

    for (int engineIndex = 0; engineIndex < engines.size(); engineIndex++) {
        // TODO: Run a cycle, get the output of that cycle and put it in the
        // Station
    }
}

} // namespace Cicero