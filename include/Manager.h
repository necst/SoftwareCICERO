#pragma once

#include "Buffers.h"
#include "Core.h"
#include <vector>

namespace Cicero {

class Manager {
  private:
    // Components
    Buffers *buffers;
    Core *core;

    // Manager signal
    unsigned short currentChar;
    unsigned short CC_ID;

    unsigned short HEAD;
    std::vector<bool> CCIDBitmap;
    unsigned short size;

    // settings
    bool verbose;

  public:
    Manager(Buffers *b, Core *c, unsigned short W, bool dbg = false);
    void updateBitmap();
    unsigned short checkBitmap();

    int mod(int k, int n);

    bool runMultiChar(const char *input);
};

} // namespace Cicero
