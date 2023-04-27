#include "CiceroMulti.h"

int main(void) {

    auto CICERO = Cicero::CiceroMulti(1, true);
    CICERO.setProgram("./test/programs/1");

    if (CICERO.match("RKMS"))
        printf("regex %d 	, input %d (len: %d)	, match True\n", 0, 0,
               256);
    else
        printf("regex %d 	, input %d (len: %d)	, match False\n", 0, 0,
               256);

    return 0;
}
