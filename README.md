# Softcore CICERO

Software simulator for the [CICERO](https://github.com/necst/cicero) domain specific architecture.

This is a fork from [Valentina Sona](https://github.com/ValentinaSona) original [git repository](https://github.com/ValentinaSona/SoftwareCICERO)

## Build

To build the library and the example application, install CMake, Make and a C++ compiler

```bash
mkdir build
cd build
cmake ..
make

# Optionally run tests
make test
```

## Run example

```bash
./build/cicero ./test/programs/1
```

## Use the library

Instantiate a CICERO object specifying:

1. **Character window (min 1, multichar only)**: number of active characters in the sliding window.
2. **Verbose setting (true/false)**: print execution information or match silently.

```cpp
#include "CiceroMulti.h"

auto CICERO = Cicero::CiceroMulti(1, true);
```

The simulator takes in input the program files compiled by [CICERO's compiler](https://github.com/necst/cicero_compiler).

Set the program (can be set multiple times in the object lifetime) and run the desired matches:

```cpp
CICERO.setProgram("program/to/run");

bool result = CICERO.match("RKMS");
bool result2 = CICERO.match("RACS");
```

## Paper Citation

If you find this repository useful, please use the following citations:

```
@inproceedings{somaini2025cicero,
    title = {Combining MLIR Dialects with Domain-Specific Architecture for Efficient Regular Expression Matching},
    author = {Andrea Somaini and Filippo Carloni and Giovanni Agosta and Marco D. Santambrogio and Davide Conficconi},
    year = 2025,
    month = {mar},
    booktitle={2025 IEEE/ACM International Symposium on Code Generation and Optimization (CGO)}
 } 
```
