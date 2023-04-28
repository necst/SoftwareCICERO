#include "Core.h"
#include "Const.h"

#include <cstddef>
#include <cstdio>

namespace Cicero {

Core::Core(Instruction *p, bool dbg) {
    program = p;
    verbose = dbg;
    reset();
}

void Core::reset() {
    accept = false;
    valid = false;
    running = true;
    pipelineRegister12 = nullptr;
    pipelineRegister23 = nullptr;
    outStage1 = CoreOUT();
    outStage2 = CoreOUT();
}

bool Core::isAccepted() const { return accept; }
bool Core::isValid() const { return valid; }
bool Core::isRunning() const { return running; }

bool Core::isStage2Ready() { return pipelineRegister12 != nullptr; }

bool Core::isStage3Ready() {
    return pipelineRegister23 != nullptr &&
           pipelineRegister23->getType() == SPLIT;
}

Instruction *Core::getPipelineRegister12() { return pipelineRegister12; }
Instruction *Core::getPipelineRegister23() { return pipelineRegister23; }
CoreOUT Core::getOutStage1() { return outStage1; }
CoreOUT Core::getOutStage2() { return outStage2; }

void Core::stage1Stall() {
    // Sets pipelineRegister12 to NULL if
    pipelineRegister12 = nullptr;
}

// Multichar version
void Core::stage1(CoreOUT bufferOUT) {
    pipelineRegister12 = &program[bufferOUT.getPC()];
    outStage1 = bufferOUT;
    if (verbose) {
        printf("\t(PC%d)(CC_ID%d)(S1) ", bufferOUT.getPC(),
               bufferOUT.getCC_ID());
        pipelineRegister12->printType(bufferOUT.getPC());
        printf("\n");
    }
}

void Core::stage2Stall() { pipelineRegister23 = nullptr; }

CoreOUT Core::stage2(CoreOUT sCO12, Instruction *stage12, char currentChar) {
    // Stage 2: get next PC and handle ACCEPT
    pipelineRegister23 = stage12->getType() == SPLIT ? stage12 : nullptr;
    outStage2 = sCO12;
    CoreOUT newPC;
    running = true;
    accept = false;
    valid = false;

    if (verbose) {
        printf("\t(PC%d)(CC_ID%d)(S2) ", sCO12.getPC(), sCO12.getCC_ID());
        stage12->printType(sCO12.getPC());
        printf("\n");
    }

    switch (stage12->getType()) {

    case ACCEPT:
        if (currentChar == '\0')
            accept = true;
        newPC = CoreOUT();
        break;

    case SPLIT:
        valid = true;
        newPC = CoreOUT(sCO12.getPC() + 1, sCO12.getCC_ID());
        break;

    case MATCH:
        if (char(stage12->getData()) == currentChar) {
            valid = true;
            newPC = CoreOUT(sCO12.getPC() + 1, sCO12.getCC_ID() + 1);
            if (verbose) {
                printf("\t\tCharacters matched: input %c to %c\n", currentChar,
                       char(stage12->getData()));
            }
        } else {
            if (verbose) {
                printf("\t\tCharacters not matched: input %c to %c\n",
                       currentChar, char(stage12->getData()));
            }
            newPC = CoreOUT();
        }
        break;

    case JMP:
        valid = true;
        newPC = (CoreOUT(stage12->getData(), sCO12.getCC_ID()));
        break;

    case END_WITHOUT_ACCEPTING:
        running = false;
        newPC = CoreOUT();
        break;

    case MATCH_ANY:
        valid = true;
        if (verbose) {
            printf("\t\tCharacters matched: input %c to ANY\n", currentChar);
        }
        newPC = (CoreOUT(sCO12.getPC() + 1, (sCO12.getCC_ID() + 1)));
        break;

    case ACCEPT_PARTIAL:
        accept = true;
        newPC = CoreOUT();
        break;

    case NOT_MATCH:
        if (char(stage12->getData()) != currentChar) {
            valid = true;
            newPC = CoreOUT(sCO12.getPC() + 1, sCO12.getCC_ID());
        } else {
            newPC = CoreOUT();
        }
        break;

    default:
        fprintf(stderr, "[X] Malformed instruction found.");
        newPC = CoreOUT();
        break;
    }
    return newPC;
}

CoreOUT Core::stage3(CoreOUT sCO23, Instruction *stage23) {
    if (verbose) {
        printf("\t(PC%d)(CC_ID%d)(S3)", sCO23.getPC(), sCO23.getCC_ID());
        stage23->printType(sCO23.getPC() + 1);
        printf("\n");
    }
    CoreOUT newPC = CoreOUT(stage23->getData(), sCO23.getCC_ID());

    return newPC;
}
CoreOUT Core::runClock(char inputChar) {}

} // namespace Cicero
