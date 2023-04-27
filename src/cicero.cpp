#include "CiceroMulti.h"
#include <cstdio>

int main(int argc, char **argv) {

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <path/to/program>", argv[0]);
        return -1;
    }

    auto CICERO = Cicero::CiceroMulti(1, true);
    CICERO.setProgram(argv[1]);

    bool matchResult = CICERO.match("RKMS");

    if (matchResult)
        printf("regex %d 	, input %d (len: %d)	, match True\n", 0, 0,
               256);
    else
        printf("regex %d 	, input %d (len: %d)	, match False\n", 0, 0,
               256);

    return 0;
}
