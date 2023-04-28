#pragma once

#include "Buffers.h"
#include "Core.h"
#include "Instruction.h"
#include <memory>
#include <vector>

namespace Cicero {

class Manager {
  private:
    // Components
    std::unique_ptr<Buffers> buffers;
    std::unique_ptr<Core> core;

    // Manager signal
    unsigned short currentChar;
    unsigned short CC_ID;

    unsigned short HEAD;
    std::vector<bool> CCIDBitmap;
    unsigned short size;

    // settings
    bool verbose;

  public:
    Manager(Instruction *program, unsigned short W, bool dbg = false);

    void updateBitmap();
    unsigned short checkBitmap();

    int mod(int k, int n);

    bool runMultiChar(const char *input);
};

} // namespace Cicero
