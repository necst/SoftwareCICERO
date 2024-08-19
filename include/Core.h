#pragma once

#include "Buffers.h"
#include "Const.h"
#include "CoreOUT.h"
#include "Instruction.h"

#include <string>

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
    Instruction *pipelineRegister12;
    Instruction *pipelineRegister23;
    CoreOUT outStage1;
    CoreOUT outStage2;

    // settings
    bool verbose;

  public:
    Core(Instruction *p, bool dbg = false);
    void reset();

    bool isAccepted() const;
    bool isValid() const;
    bool isRunning() const;

    bool isStage2Ready();
    bool isStage3Ready();

    Instruction *getPipelineRegister12();
    Instruction *getPipelineRegister23();
    CoreOUT getOutStage1();
    CoreOUT getOutStage2();

    void stage1Stall();
    void stage1(CoreOUT bufferOUT);

    void stage2Stall();
    CoreOUT stage2(CoreOUT sCO12, Instruction *stage12, char currentChar);

    CoreOUT stage3(CoreOUT sCO23, Instruction *stage23);
    ClockResult runClock(std::string input, int currentWindowIndex,
                         int currentBufferIndex, int windowSize,
                         Buffers *buffers);
};

} // namespace Cicero
