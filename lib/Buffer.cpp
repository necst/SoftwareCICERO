#include "Buffers.h"

#include <cstdio>
#include <queue>

namespace Cicero {

// Container for all the buffers - permits to instantiate a variable number of
// buffers.
Buffers::Buffers(int n) {
    size = n;
    buffers.reserve(n);
    for (int i = 0; i < n; i++) {
        buffers.push_back(std::queue<unsigned short>());
    }
}

void Buffers::flush() {
    for (int i = 0; i < size; i++) {
        while (!buffers[i].empty()) {
            buffers[i].pop();
        }
    }
}

bool Buffers::isEmpty(unsigned short CC_ID) {
    return buffers[(CC_ID) % size].empty();
}

// Expects to be told which is the buffer holding first character of sliding
// window.
bool Buffers::hasInstructionReady(unsigned short HEAD) {
    for (int i = 0; i < size - 1;
         i++) { // Excludes the last buffer of the sliding window.
        if (!buffers[(HEAD + i) % size].empty()) {
            return true;
        }
    }
    return false;
}

unsigned short Buffers::getFirstNotEmpty(unsigned short HEAD) {
    for (int i = 0; i < size - 1; i++) { // Cannot return the inactive one.

        if (!buffers[(HEAD + i) % size].empty()) {
            return (HEAD + i) % size;
        }
    }
    return size; // To be considered as all empty.
}

bool Buffers::areAllEmpty() {
    bool areEmpty = true;
    for (int i = 0; i < size; i++) {
        if (!buffers[i].empty()) {
            areEmpty = false;
            break;
        }
    }
    return areEmpty;
}

CoreOUT Buffers::getPC(unsigned short CC_ID) {
    CoreOUT PC = CoreOUT(buffers[(CC_ID) % size].front(), CC_ID);
    return PC;
}

CoreOUT Buffers::popPC(unsigned short CC_ID) {
    CoreOUT PC = CoreOUT(buffers[(CC_ID) % size].front(), CC_ID);
    buffers[(CC_ID) % size].pop();
    return PC;
}

void Buffers::pushTo(unsigned short CC_ID, unsigned short PC) {

    if (CC_ID < size) {
        buffers[(CC_ID) % size].push(PC);
    } else
        fprintf(stderr, "[X] Pushing to non-existing buffer %d.\n", CC_ID);
}

} // namespace Cicero
