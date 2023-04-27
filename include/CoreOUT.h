#pragma once

namespace Cicero {

// Models the output of the Core, which is the new PC plus the bit indicating
// whether the returned instruction refers to the active character or not.
class CoreOUT {
  private:
    unsigned short PC;
    unsigned short CC_ID;

  public:
    CoreOUT();
    CoreOUT(unsigned short PC);
    CoreOUT(unsigned short PC, unsigned short CC_ID);

    unsigned short getPC();
    unsigned short getCC_ID();
};

} // namespace Cicero
