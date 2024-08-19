#pragma once

namespace Cicero {

// Wrapper around the 16bit instruction for easy retrieval of type, data and
// easy printing.
class Instruction {
  private:
    unsigned short instr;

  public:
    Instruction();

    Instruction(unsigned short instruction);
    unsigned short getType();
    unsigned short getData();

    void printType(int PC);
    void print(int pc);
};

} // namespace Cicero
