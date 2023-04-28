#include "CiceroMulti.h"
#include <fstream>
#include <ios>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

const int PROGRAMS_COUNT = 1308;
const int INPUT_COUNT = 100;

struct CorrectResult {
    int regexNumber;
    int inputNumber;
    bool matchResult;

    static CorrectResult parseRow(std::string row) {
        std::istringstream rowStream(row);

        std::string buffer;

        if (!std::getline(rowStream, buffer, ',')) {
            std::cerr << "Error while parsing regexNumber CSV for row = " << row
                      << std::endl;
            throw -1;
        }

        int regexNumber = std::stoi(buffer);

        if (!std::getline(rowStream, buffer, ',')) {
            std::cerr << "Error while parsing inputNumber CSV for row = " << row
                      << std::endl;
            throw -1;
        }

        int inputNumber = std::stoi(buffer);

        if (!std::getline(rowStream, buffer)) {
            std::cerr << "Error while parsing result CSV for row = " << row
                      << std::endl;
            throw -1;
        }

        bool result;

        if (buffer == "True") {
            result = true;
        } else if (buffer == "False") {
            result = false;
        } else {
            std::cerr << "Invalid result value parsed in CSV. Expected 'True' "
                         "or 'False', found '"
                      << buffer << "'.\n Row = " << row << std::endl;
            throw -1;
        }

        return {regexNumber, inputNumber, result};
    }
};

std::vector<CorrectResult> getCorrectResults() {
    std::string inputStringsPath =
        TEST_INPUT_PATH + std::string("protomata_results.csv");

    std::ifstream inputResultsFile(inputStringsPath, std::ios_base::in);

    if (!inputResultsFile.is_open()) {
        std::cerr << "Unable to open protomata_results.csv file with input "
                     "strings.\n";
        throw -1;
    }

    std::vector<CorrectResult> returnValue;

    std::string buffer;
    while (std::getline(inputResultsFile, buffer)) {
        returnValue.push_back(CorrectResult::parseRow(buffer));
    }

    return returnValue;
}

int main() {
    auto cicero = Cicero::CiceroMulti(2);

    std::vector<std::string> inputStrings;

    std::string inputStringsPath = TEST_INPUT_PATH + std::string("strings.txt");

    std::ifstream inputStringFile(inputStringsPath, std::ios_base::in);

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

    inputStringFile.close();

    auto correctResults = getCorrectResults();
    int correctResultIndex = 0;

    for (int i = 0; i <= PROGRAMS_COUNT; i++) {

        std::cout << "\rRunning program number " << i;
        std::cout.flush();

        std::string programPath =
            TEST_INPUT_PATH + std::string("programs/") + std::to_string(i);

        cicero.setProgram(programPath.c_str());

        if (!cicero.isProgramSet()) {
            std::cerr << "Unable to load program " << programPath << std::endl;
            continue;
        }

        for (int j = 0; j < inputStrings.size(); j++) {
            auto inputString = inputStrings[j];

            if (correctResultIndex >= correctResults.size()) {
                std::cerr << "Regex number " << i << "; input number " << j
                          << "; resultIndex = " << correctResultIndex
                          << "; No more corrected results in CSV file??\n";
                return -1;
            }

            auto correctResult = correctResults[correctResultIndex++];

            if (correctResult.regexNumber != i ||
                correctResult.inputNumber != j) {
                std::cerr << "Regex number " << i << "; input number " << j
                          << "; resultIndex = " << correctResultIndex
                          << "; CorrectResult in CSV does not correspond to "
                             "regex number or input number.\n";
                return -1;
            }

            bool matchResult = cicero.match(inputString.c_str());
            if (matchResult != correctResult.matchResult) {
                std::cerr << "Regex number " << i << "; input number " << j
                          << "; resultIndex = " << correctResultIndex
                          << "; CorrectResult do not correspond. Expected "
                          << correctResult.matchResult << " but got "
                          << matchResult;
                return -1;
            }
        }
    }
}