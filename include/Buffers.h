#pragma once

#include "CoreOUT.h"
#include <queue>
#include <vector>

namespace Cicero {

// Container for all the buffers - permits to instantiate a variable number of
// buffers.
class Buffers {
  private:
    std::vector<std::queue<unsigned short>> buffers;
    int HEAD;
    int size; // 2**W
  public:
    Buffers(int n);
    void flush();

    void slide(unsigned short slide);

    // Expects to be told which is the buffer holding first character of sliding
    // window.
    bool hasInstructionReady(unsigned short HEAD);
    unsigned short getFirstNotEmpty(unsigned short HEAD);
    bool isEmpty(unsigned short CC_ID);
    bool areAllEmpty();

    CoreOUT getPC(unsigned short CC_ID);
    CoreOUT popPC(unsigned short CC_ID);

    void pushTo(unsigned short CC_ID, unsigned short PC);
};

} // namespace Cicero
