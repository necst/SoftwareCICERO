#ifndef CICEROMULTI_H
#define CICEROMULTI_H

#include <iostream>
#include <queue>
#include <cstring>
#include <vector>
#include <cmath>

#include "CoreOUT.h"
#include "Instruction.h"
#include "Buffers.h"
#include "Core.h"
#include "Manager.h"
#include "Const.h"

namespace Cicero
{
	// Wrapper class that holds and inits all components.
	class CiceroMulti
	{
	private:
		// Components
		Instruction program[INSTR_MEM_SIZE];
		Buffers buffers = Buffers(2);
		Core core = Core(&program[0]);
		Manager manager = Manager(&buffers, &core, 2);

		// Settings
		bool verbose = true;
		bool hasProgram = false;

	public:
		CiceroMulti(unsigned short W = 1, bool dbg = false);

		void setProgram(const char *filename);
		bool isProgramSet();

		bool match(const char *input);
	};
}
#endif
