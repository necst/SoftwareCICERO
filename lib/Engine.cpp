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
    currentBufferIndex = 0;
    CCIDBitmap.reserve(W);
    for (int i = 0; i < W; i++) {
        CCIDBitmap.push_back(false);
    }
}

void Engine::updateBitmap() {
    // Check buffers
    for (unsigned short i = 0; i < CCIDBitmap.size(); i++) {
        CCIDBitmap[i] = (!buffers->isEmpty(i)) |
                        ((core->getOutStage1().getCC_ID() == i) &
                         (core->getPipelineRegister12() != NULL)) |
                        ((core->getOutStage2().getCC_ID() == i) &
                         (core->getPipelineRegister23() != NULL));
    }
}

unsigned short Engine::checkBitmap() {

    unsigned short slide = 0;
    for (unsigned short i = 0; i < CCIDBitmap.size(); i++) {
        if (CCIDBitmap[(currentBufferIndex + i) % size] == 0)
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
    currentWindowIndex = 0;
    currentBufferIndex = 0;

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
        switch (runClock()) {
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

ClockResult Engine::runClock() {
    currentClockCycle++;

    if (verbose)
        printf("[CC%d] Window first character: %c\n", currentClockCycle,
               input[currentWindowIndex]);

    ClockResult coreResult = core->runClock(
        input, currentWindowIndex, currentBufferIndex, size, buffers.get());

    // We have already accepted/refused, early quit.
    if (coreResult != CONTINUE) {
        return coreResult;
    }

    updateBitmap();

    if (checkBitmap() != 0) { // Conditions for sliding the window.

        if (verbose)
            printf("\t\t%x Threads are inactive, sliding window. New first "
                   "char in window: %c\n",
                   checkBitmap(), input[currentWindowIndex]);
        currentWindowIndex += checkBitmap(); // Move the window + i
        currentBufferIndex = (currentBufferIndex + checkBitmap()) % size;
    }

    // End the cycle AFTER having processed the '\0' (which can be consumed
    // by an ACCEPT) or if no more instructions are left to be processed.
    if (input.size() < currentWindowIndex ||
        (buffers->areAllEmpty() && !core->isStage2Ready() &&
         !core->isStage3Ready()))
        return REFUSED;

    return CONTINUE;
}

} // namespace Cicero
