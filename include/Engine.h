#pragma once

#include "Buffers.h"
#include "Core.h"
#include "Instruction.h"
#include <memory>
#include <string>
#include <vector>

namespace Cicero {

class Engine {
  private:
    // Components
    std::unique_ptr<Buffers> buffers;
    std::unique_ptr<Core> core;

    std::string input;
    int currentClockCycle;

    // Engine signal
    unsigned short currentWindowIndex;

    unsigned short currentBufferIndex;
    // Bitmap containing which buffers are ready to execute some threads
    std::vector<bool> CCIDBitmap;
    unsigned short windowSize;

    // settings
    bool verbose;

    ClockResult runClock();

    void updateBitmap();
    unsigned short checkBitmap();

  public:
    Engine(Instruction *program, unsigned short W, bool dbg = false);

    static int mod(int k, int n);

    void reset(std::string newInput);

    bool runMultiChar(std::string _input);
};

} // namespace Cicero
