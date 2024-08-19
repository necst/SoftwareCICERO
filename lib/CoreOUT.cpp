#include "CoreOUT.h"

namespace Cicero {

CoreOUT::CoreOUT() {
    PC = 0;
    CC_ID = 0;
};

CoreOUT::CoreOUT(unsigned short PC) { this->PC = PC; }

CoreOUT::CoreOUT(unsigned short PC, unsigned short CC_ID) {
    this->PC = PC;
    this->CC_ID = CC_ID;
}

unsigned short CoreOUT::getPC() { return PC; }
unsigned short CoreOUT::getCC_ID() { return CC_ID; }

} // namespace Cicero
