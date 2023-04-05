# Softcore CICERO

Software simulator for the [CICERO](https://github.com/necst/cicero) domain specific architecture.

## Usage

Instantiate a CICERO object, with either the base or the multi-character architecture.

```cpp
#include "CiceroBase.h"
#include "CiceroMulti.h"

CiceroBase::SoftwareCICERO CICERO = CiceroBase::SoftwareCICERO(true);
CiceroMulti::SoftwareCICERO CICERO = CiceroMulti::SoftwareCICERO(1, true);
```

- **Verbose setting (true/false)**: print execution information or match silently.
- **Character window (min 1, multichar only)**: number of active characters in the sliding window.

The simulator takes in input the program files compiled by [CICERO's compiler](https://github.com/necst/cicero_compiler). 

Set the program (can be set multiple times in the object lifetime) and run the desired matches:

```cpp
CICERO.setProgram("program/to/run");

bool result = CICERO.match("RKMS");
bool result2 = CICERO.match("RACS");

```


