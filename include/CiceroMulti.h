#ifndef CICEROMULTI_H
#define CICEROMULTI_H

#include <cmath>
#include <cstring>
#include <iostream>
#include <memory>
#include <queue>
#include <vector>

#include "Buffers.h"
#include "Const.h"
#include "Core.h"
#include "CoreOUT.h"
#include "Instruction.h"
#include "Manager.h"

namespace Cicero {
// Wrapper class that holds and inits all components.
class CiceroMulti {
  private:
    // Components
    Instruction program[INSTR_MEM_SIZE];

    std::unique_ptr<Manager> manager;

    // Settings
    bool verbose = true;
    bool hasProgram = false;

  public:
    CiceroMulti(unsigned short W = 1, bool dbg = false);

    void setProgram(const char *filename);
    bool isProgramSet();

    bool match(const char *input);
};
} // namespace Cicero
#endif
