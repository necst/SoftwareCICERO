#include "Engine.h"
#include "Buffers.h"
#include "Instruction.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <utility>

namespace Cicero {

Engine::Engine(Instruction *program, unsigned short W, bool dbg) {
    core = std::make_unique<Core>(program, dbg);
    buffers = std::make_unique<Buffers>(W);
    verbose = dbg;
    size = W;
    HEAD = 0;
    CCIDBitmap.reserve(W);
    for (int i = 0; i < W; i++) {
        CCIDBitmap.push_back(false);
    }
}

void Engine::updateBitmap() {
    // Check buffers
    for (unsigned short i = 0; i < CCIDBitmap.size(); i++) {
        CCIDBitmap[i] =
            (!buffers->isEmpty(i)) |
            ((core->getCO12().getCC_ID() == i) & (core->getStage12() != NULL)) |
            ((core->getCO23().getCC_ID() == i) & (core->getStage23() != NULL));
    }
}

unsigned short Engine::checkBitmap() {

    unsigned short slide = 0;
    for (unsigned short i = 0; i < CCIDBitmap.size(); i++) {
        if (CCIDBitmap[(HEAD + i) % size] == 0)
            slide += 1;
        else
            break;
    }
    return slide;
}

int Engine::mod(int k, int n) { return ((k %= n) < 0) ? k + n : k; }

bool Engine::runMultiChar(std::string _input) {

    input = std::move(_input);

    // Signals handled by Engine
    // In MultiChar it refers to first character in sliding window.
    currentChar = 0;
    HEAD = 0;

    currentClockCycle = 0;

    // Reset to known empty state.
    core->reset();
    buffers->flush();
    /*for (int i = 0; i < size; i++){
      CCIDBitmap.push_back(false);
    }*/
    // Load first instruction PC.
    buffers->pushTo(0, 0);
    CCIDBitmap[0] = true;

    if (verbose)
        printf("\nInitiating match of string %s\n", input.c_str());

    // Simulate clock cycle
    while (!core->isAccepted() && core->isRunning()) {
        switch(runClock()) {
        case CONTINUE:
            break;
        case ACCEPTED:
            return true;
        case REFUSED:
            return false;
        }
    }

    return false;
}

EngineClockResult Engine::runClock() {
    CoreOUT newPC;

    if (verbose)
        printf("[CC%d] Window first character: %c\n", currentClockCycle,
               input[currentChar]);

    /* READ
         * To simulate the stages being concurrent, all values used by stages 2
         * and 3 must be read at the start of the cycle, emulating values being
         * read on rise. */

    // Check at the start whether each stage meets the conditions for being
    // executed.
    bool stage1Ready = buffers->hasInstructionReady(HEAD);
    bool stage2Ready = core->isStage2Ready();
    bool stage3Ready = core->isStage3Ready();

    if (verbose)
        printf("\tStages deemed ready: %x, %x, %x\n", stage1Ready,
               stage2Ready, stage3Ready);
    // Save the inter-stage registers for use.
    Instruction *s12 = core->getStage12();
    Instruction *s23 = core->getStage23();
    CoreOUT CO12 = core->getCO12();
    CoreOUT CO23 = core->getCO23();

    /* EXEC */

    // Stage 1: retrieve newPC from active buffer and load instruction.
    if (stage1Ready)
        core->stage1(buffers->getPC(buffers->getFirstNotEmpty(HEAD)));
    else {
        // Set intermediate registers to zero
        core->stage1();
    }

    // Stage 2: sets valid, running and accept signals.
    if (!stage2Ready) {
        // Set intermediate registers to zero
        core->stage2();
    } else {
        int inputIndex = currentChar + mod((CO12.getCC_ID() - HEAD), (size));

        if (inputIndex > input.size()) {
            // We are out of the string! Do not create a new thread i.e. not add
            // anything to the buffers
        } else {
            newPC = core->stage2(CO12, s12, input[inputIndex]);

            // Handle the returned value, if it's a valid one.
            if (core->isValid()) {
                if (verbose)
                    printf("\t\tPushing PC%d to FIFO%d\n", newPC.getPC(),
                           newPC.getCC_ID() % size);
                // Push to correct buffer
                buffers->pushTo(newPC.getCC_ID() % size, newPC.getPC());
                // Invalid values that must be handled are returned by ACCEPT,
                // ACCEPT_PARTIAL and END_WITHOUT_ACCEPTING. Apart from these,
                // the only way for a computation to end is by reaching end of
                // string without ACCEPT.
            } else if (core->isAccepted()) {
                return ACCEPTED;
            } else if (!core->isRunning()) {
                return REFUSED;
            }
        }

    } // Empty run to clear signals

    // Stage 3: Only executed by a SPLIT instruction
    if (stage3Ready) {

        newPC = core->stage3(CO23, s23);

        if (verbose)
            printf("\t\tPushing PC%d to FIFO%x\n", newPC.getPC(),
                   newPC.getCC_ID() % size);
        // Push to correct buffer
        buffers->pushTo(newPC.getCC_ID() % size, newPC.getPC());
    }

    /* WRITEBACK
         * Values should only be modified at the end of the clock cycle */

    // Only if stage 1 was executed, consume the instruction that was loaded
    // from the buffer. Otherwise, it would risk consuming a value added by
    // stage 2 in the same cycle.
    if (stage1Ready) {
        if (verbose)
            printf(
                "\t\tConsumed PC%d from FIFO%d, relating to character %c\n",
                core->getCO12().getPC(), core->getCO12().getCC_ID(),
                input[currentChar + mod((CO12.getCC_ID() - HEAD), (size))]);

        buffers->popPC(core->getCO12().getCC_ID());
        if (verbose)
            printf("\t\tNext PC from FIFO%d: %d\n",
                   buffers->getFirstNotEmpty(HEAD),
                   buffers->getPC(buffers->getFirstNotEmpty(HEAD)).getPC());
    }

    updateBitmap();

    if (checkBitmap() != 0) { // Conditions for sliding the window.

        if (verbose)
            printf("\t\t%x Threads are inactive, sliding window. New first "
                   "char in window: %c\n",
                   checkBitmap(), input[currentChar]);
        currentChar += checkBitmap(); // Move the window + i
        HEAD = (HEAD + checkBitmap()) % size;
    }

    currentClockCycle++;
    // End the cycle AFTER having processed the '\0' (which can be consumed
    // by an ACCEPT) or if no more instructions are left to be processed.
    if (input.size() < currentChar ||
        (buffers->areAllEmpty() && !core->isStage2Ready() &&
         !core->isStage3Ready()))
        return REFUSED;

    return CONTINUE;
}

} // namespace Cicero
