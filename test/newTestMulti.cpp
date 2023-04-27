#include "CiceroMulti.h"
#include <cstdio>
#include <fstream>
#include <ios>
#include <iostream>

const int PROGRAMS_COUNT = 1308;
const int INPUT_COUNT = 100;

int main() {
    auto cicero = Cicero::CiceroMulti(2);

    std::vector<std::string> inputStrings;

    std::ifstream inputStringFile("strings.txt", std::ios_base::in);

    if (!inputStringFile.is_open()) {
        std::cerr << "Unable to open strings.txt file with input strings.\n";
        return -1;
    }

    std::string buffer;
    for (int j = 0; j < INPUT_COUNT; j++) {
        if (!std::getline(inputStringFile, buffer)) {
            std::cerr << "strings.txt file is not complete? Unable to read at "
                      << j << std::endl;
            return -1;
        }
        inputStrings.push_back(buffer);
    }

    for (int i = 0; i <= PROGRAMS_COUNT; i++) {

        std::string programPath = "./programs/" + std::to_string(i);

        cicero.setProgram(programPath.c_str());

        if (!cicero.isProgramSet()) {
            std::cerr << "Unable to load program " << programPath << std::endl;
            continue;
        }

        for (int j = 0; j < inputStrings.size(); j++) {
            auto inputString = inputStrings[j];
            bool result = cicero.match(inputString.c_str());
            if (result) {
                printf("regex %d 	, input %d (len: %zu)	, match True\n",
                       i, j, inputString.size());
            } else {
                printf(
                    "regex %d 	, input %d (len: %zu)	, match False\n", i, j,
                    inputString.size());
            }
        }
    }
}