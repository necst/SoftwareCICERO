#include "CiceroMulti.h"
#include "Buffers.h"
#include <cmath>
#include <cstring>
#include <iostream>
#include <memory>
#include <queue>
#include <vector>

namespace Cicero {

// Wrapper class that holds and inits all components.
CiceroMulti::CiceroMulti(unsigned short W, bool dbg) {

    if (W == 0)
        W = 1;

    hasProgram = false;
    verbose = dbg;

    manager = std::make_unique<Manager>(program, W + 1, dbg);
}

void CiceroMulti::setProgram(const char *filename) {
    FILE *fp = fopen(filename, "r");
    unsigned short instr;
    int i;

    if (fp != NULL) {

        if (verbose)
            printf("Reading program file: \n\n");

        for (i = 0; i < INSTR_MEM_SIZE && !feof(fp); i++) {

            fscanf(fp, "%hx", &instr);
            program[i] = Instruction(instr);
            fscanf(fp, "\n");

            // Pretty print instructions
            if (verbose)
                program[i].print(i);
        }

        if (i == INSTR_MEM_SIZE && !feof(fp)) {
            fprintf(
                stderr,
                "[X] Program memory exceeded. Only the first %x instructions "
                "were read.\n",
                INSTR_MEM_SIZE);
        }

        hasProgram = true;
        fclose(fp);
    } else {
        hasProgram = false;
        fprintf(stderr, "[X] Could not open program file %s for reading.\n",
                filename);
    }
};

bool CiceroMulti::CiceroMulti::isProgramSet() { return hasProgram; }

bool CiceroMulti::match(const char *input) {

    if (!hasProgram) {
        fprintf(stderr,
                "[X] No program is loaded to match the string against.\n");
        return false;
    }

    return manager->runMultiChar(input);
}

} // namespace Cicero
