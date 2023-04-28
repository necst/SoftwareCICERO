#pragma once

#include "Buffers.h"
#include "Core.h"
#include "Instruction.h"
#include <memory>
#include <vector>
#include <string>

namespace Cicero {

enum EngineClockResult {
    CONTINUE,
    ACCEPTED,
    REFUSED
};

class Engine {
  private:
    // Components
    std::unique_ptr<Buffers> buffers;
    std::unique_ptr<Core> core;

    std::string input;
    int currentClockCycle;

    // Engine signal
    unsigned short currentChar;

    unsigned short HEAD;
    std::vector<bool> CCIDBitmap;
    unsigned short size;

    // settings
    bool verbose;

    EngineClockResult runClock();

  public:
    Engine(Instruction *program, unsigned short W, bool dbg = false);

    void updateBitmap();
    unsigned short checkBitmap();

    static int mod(int k, int n);

    bool runMultiChar(std::string _input);
};

} // namespace Cicero
