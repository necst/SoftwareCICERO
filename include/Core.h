#pragma once

#include "CoreOUT.h"
#include "Instruction.h"

namespace Cicero {

class Core {
  private:
    Instruction
        *program; // Stage 1 accesses program memory to retrieve instruction.

    // Signals (as seen from HDL)
    bool accept;
    bool valid;
    bool running;

    // Inter-phase registers
    Instruction *s12;
    Instruction *s23;
    CoreOUT CO12;
    CoreOUT CO23;

    // settings
    bool verbose;

  public:
    Core(Instruction *p, bool dbg = false);
    void reset();

    bool isAccepted();
    bool isValid();
    bool isRunning();

    bool isStage2Ready();
    bool isStage3Ready();

    Instruction *getStage12();
    Instruction *getStage23();
    CoreOUT getCO12();
    CoreOUT getCO23();

    void stage1();
    // Multichar version
    void stage1(CoreOUT bufferOUT);

    void stage2();
    CoreOUT stage2(CoreOUT sCO12, Instruction *stage12, char currentChar);

    CoreOUT stage3(CoreOUT sCO23, Instruction *stage23);
};

} // namespace Cicero
