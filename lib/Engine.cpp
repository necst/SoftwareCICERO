#include "Engine.h"
#include "Buffers.h"
#include "Instruction.h"

#include <cstdio>
#include <memory>
#include <string>
#include <utility>

namespace Cicero {

Engine::Engine(Instruction *program, unsigned short W, bool dbg) {
    core = std::make_unique<Core>(program, dbg);
    buffers = std::make_unique<Buffers>(W);
    verbose = dbg;
    windowSize = W;
    currentBufferIndex = 0;
    CCIDBitmap = std::vector(windowSize, false);
}

void Engine::updateBitmap() {
    // Check buffers
    for (unsigned short i = 0; i < CCIDBitmap.size(); i++) {
        CCIDBitmap[i] = (!buffers->isEmpty(i)) |
                        ((core->getOutStage1().getCC_ID() == i) &
                         (core->getPipelineRegister12() != nullptr)) |
                        ((core->getOutStage2().getCC_ID() == i) &
                         (core->getPipelineRegister23() != nullptr));
    }
}

unsigned short Engine::checkBitmap() {

    unsigned short slide = 0;
    for (unsigned short i = 0; i < windowSize; i++) {
        if (CCIDBitmap[(currentBufferIndex + i) % windowSize] == 0)
            slide += 1;
        else
            break;
    }
    return slide;
}

int Engine::mod(int k, int n) { return ((k %= n) < 0) ? k + n : k; }

void Engine::reset(std::string newInput) {
    input = std::move(newInput);

    currentWindowIndex = 0;
    currentBufferIndex = 0;
    currentClockCycle = 0;

    core->reset();
    buffers->flush();

    // Load first instruction PC.
    buffers->pushTo(0, 0);

    CCIDBitmap[0] = true;
    for (int i = 1; i < windowSize; i++) {
        CCIDBitmap[i] = false;
    }
}

bool Engine::runMultiChar(std::string _input) {

    reset(_input);

    if (verbose)
        printf("\nInitiating match of string %s\n", input.c_str());

    // Simulate clock cycle
    while (core->isRunning()) {
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

    ClockResult coreResult =
        core->runClock(input, currentWindowIndex, currentBufferIndex,
                       windowSize, buffers.get());

    // We have already accepted/refused, early quit.
    if (coreResult != CONTINUE) {
        return coreResult;
    }

    updateBitmap();

    if (checkBitmap() != 0) { // Conditions for sliding the window.

        currentWindowIndex += checkBitmap(); // Move the window + i
        if (verbose) {
            if (currentWindowIndex < input.size()) {
                printf("\t\t%x Threads are inactive, sliding window. New "
                       "first char in window: %c\n",
                       checkBitmap(), input[currentWindowIndex]);
            } else {
                printf("\t\t%x Threads are inactive, sliding window. New "
                       "window index is out of bounds.\n",
                       checkBitmap());
            }
        }
        currentBufferIndex = (currentBufferIndex + checkBitmap()) % windowSize;
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
