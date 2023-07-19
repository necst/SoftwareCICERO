#include "Core.h"
#include "Const.h"
#include "Engine.h"

#include <cstddef>
#include <cstdio>
#include <string>

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

ClockResult Core::runClock(std::string input, int currentWindowIndex,
                           int currentBufferIndex, int windowSize,
                           Buffers *buffers) {

    CoreOUT newPC;
    /* READ
     * To simulate the stages being concurrent, all values used by stages 2
     * and 3 must be read at the start of the cycle, emulating values being
     * read on rise. */

    // Check at the start whether each stage meets the conditions for being
    // executed.
    bool stage1Ready = buffers->hasInstructionReady(currentBufferIndex);
    bool stage2Ready = isStage2Ready();
    bool stage3Ready = isStage3Ready();

    // Save the inter-stage registers for use.
    Instruction *savedStage12 = getPipelineRegister12();
    Instruction *savedStage23 = getPipelineRegister23();
    CoreOUT savedOut12 = getOutStage1();
    CoreOUT savedOut23 = getOutStage2();

    if (verbose)
        printf("\tStages deemed ready: %x, %x, %x\n", stage1Ready, stage2Ready,
               stage3Ready);

    /* EXEC */
    // Stage 1: retrieve newPC from active buffer and load instruction.
    if (stage1Ready) {
        stage1(buffers->getPC(buffers->getFirstNotEmpty(currentBufferIndex)));
    } else {
        // Set intermediate registers to zero
        stage1Stall();
    }

    // Stage 2: sets valid, running and accept signals.
    if (!stage2Ready) {
        // Set intermediate registers to zero
        stage2Stall();
    } else {
        int inputIndex =
            currentWindowIndex +
            Engine::mod((savedOut12.getCC_ID() - currentBufferIndex),
                        (windowSize));

        if (inputIndex > input.size()) {
            // We are out of the string! Do not create a new thread i.e. not add
            // anything to the buffers
        } else {
            newPC = stage2(savedOut12, savedStage12, input[inputIndex]);

            // Handle the returned value, if it's a valid one.
            if (isValid()) {
                if (verbose)
                    printf("\t\tPushing PC%d to FIFO%d\n", newPC.getPC(),
                           newPC.getCC_ID() % windowSize);
                // Push to correct buffer
                buffers->pushTo(newPC.getCC_ID() % windowSize, newPC.getPC());
                // Invalid values that must be handled are returned by ACCEPT,
                // ACCEPT_PARTIAL and END_WITHOUT_ACCEPTING. Apart from these,
                // the only way for a computation to end is by reaching end of
                // string without ACCEPT.
            } else if (isAccepted()) {
                return ACCEPTED;
            } else if (!isRunning()) {
                return REFUSED;
            }
        }

    } // Empty run to clear signals

    // Stage 3: Only executed by a SPLIT instruction
    if (stage3Ready) {

        newPC = stage3(savedOut23, savedStage23);

        if (verbose)
            printf("\t\tPushing PC%d to FIFO%x\n", newPC.getPC(),
                   newPC.getCC_ID() % windowSize);
        // Push to correct buffer
        buffers->pushTo(newPC.getCC_ID() % windowSize, newPC.getPC());
    }

    /* WRITEBACK
     * Values should only be modified at the end of the clock cycle */

    // Only if stage 1 was executed, consume the instruction that was loaded
    // from the buffer. Otherwise, it would risk consuming a value added by
    // stage 2 in the same cycle.
    if (stage1Ready) {
        if (verbose) {
            int inputIndex =
                currentWindowIndex +
                Engine::mod((savedOut12.getCC_ID() - currentBufferIndex),
                            (windowSize));
            if (inputIndex < input.size()) {
                printf(
                    "\t\tConsumed PC%d from FIFO%d, relating to character %c\n",
                    getOutStage1().getPC(), getOutStage1().getCC_ID(),
                    input[inputIndex]);
            } else {
                printf("\t\tConsumed PC%d from FIFO%d, but character index was "
                       "out of input\n",
                       getOutStage1().getPC(), getOutStage1().getCC_ID());
            }
        }

        buffers->popPC(getOutStage1().getCC_ID());
        if (verbose)
            printf("\t\tNext PC from FIFO%d: %d\n",
                   buffers->getFirstNotEmpty(currentBufferIndex),
                   buffers->getPC(buffers->getFirstNotEmpty(currentBufferIndex))
                       .getPC());
    }

    return CONTINUE;
}

} // namespace Cicero
