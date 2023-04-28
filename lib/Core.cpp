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
    s12 = NULL;
    s23 = NULL;
    CO12 = CoreOUT();
    CO23 = CoreOUT();
}

bool Core::isAccepted() { return accept; }
bool Core::isValid() { return valid; }
bool Core::isRunning() { return running; }

bool Core::isStage2Ready() { return s12 != NULL; }

bool Core::isStage3Ready() { return s23 != NULL && s23->getType() == SPLIT; }

Instruction *Core::getStage12() { return s12; }
Instruction *Core::getStage23() { return s23; }
CoreOUT Core::getCO12() { return CO12; }
CoreOUT Core::getCO23() { return CO23; }

void Core::stage1() {
    // Sets s12 to NULL if
    s12 = NULL;
}

// Multichar version
void Core::stage1(CoreOUT bufferOUT) {
    s12 = &program[bufferOUT.getPC()];
    CO12 = bufferOUT;
    if (verbose) {
        printf("\t(PC%d)(CC_ID%d)(S1) ", bufferOUT.getPC(),
               bufferOUT.getCC_ID());
        s12->printType(bufferOUT.getPC());
        printf("\n");
    }
};

void Core::stage2() { s23 = NULL; }

CoreOUT Core::stage2(CoreOUT sCO12, Instruction *stage12, char currentChar) {
    // Stage 2: get next PC and handle ACCEPT
    if (stage12->getType() == SPLIT)
        s23 = stage12;
    else
        s23 = NULL;
    CO23 = sCO12;
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
};

CoreOUT Core::stage3(CoreOUT sCO23, Instruction *stage23) {
    if (verbose) {
        printf("\t(PC%d)(CC_ID%d)(S3)", sCO23.getPC(), sCO23.getCC_ID());
        stage23->printType(sCO23.getPC() + 1);
        printf("\n");
    }
    CoreOUT newPC = CoreOUT(stage23->getData(), sCO23.getCC_ID());

    return newPC;
};

} // namespace Cicero
