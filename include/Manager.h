#pragma once

#include "Engine.h"

namespace Cicero {

class Manager {
  private:
    std::vector<Engine> engines;

  public:
    Manager(Instruction *program, int engineCount, int windowSize);

    bool match(std::string input);
};

} // namespace Cicero