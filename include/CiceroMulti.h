#ifndef CICEROMULTI_H
#define CICEROMULTI_H

#include <iostream>
#include <queue>
#include <cstring>
#include <vector>
#include <cmath>


#define INSTR_MEM_SIZE 512 //PC is 9bits
#define BITS_INSTR_TYPE 3
#define BITS_INSTR 16

namespace CiceroMulti {

	enum InstrType {
		ACCEPT 			      = 0,
		SPLIT  			      = 1,
		MATCH  			      = 2,
		JMP	   			      = 3,
		END_WITHOUT_ACCEPTING = 4,
		MATCH_ANY 		      = 5,
		ACCEPT_PARTIAL	      = 6,
		NOT_MATCH		      = 7,
	};

	// Models the output of the Core, which is the new PC plus the bit indicating whether 
	// the returned instruction refers to the active character or not.
	class CoreOUT {
		private:
			unsigned short PC;
			unsigned short CC_ID;
		public:
			CoreOUT();
			CoreOUT(unsigned short PC);
			CoreOUT(unsigned short PC,unsigned short CC_ID);

			unsigned short getPC();
			unsigned short getCC_ID();
	};

	// Wrapper around the 16bit instruction for easy retrieval of type, data and easy printing.
	class Instruction {
		private:
			unsigned short instr;
		public: 

			Instruction();

			Instruction(unsigned short instruction);
			unsigned short getType();
			unsigned short getData();
			
			
			void printType(int PC);
			void print(int pc);
	};

	// Container for all the buffers - permits to instantiate a variable number of buffers.
	class Buffers {
		private:
			std::vector<std::queue<unsigned short>> buffers;
			int HEAD;
			int size; //2**W 
		public:
			Buffers(int n);
			void flush();

			void slide(unsigned short slide);			

			// Expects to be told which is the buffer holding first character of sliding window.
			bool hasInstructionReady(unsigned short HEAD);
			unsigned short getFirstNotEmpty(unsigned short HEAD);
			bool isEmpty(unsigned short CC_ID);
			bool areAllEmpty();

			CoreOUT getPC(unsigned short CC_ID);
			CoreOUT popPC(unsigned short CC_ID);

			void pushTo(unsigned short CC_ID, unsigned short PC);
	};



	class Core {
		private:
			Instruction* program; // Stage 1 accesses program memory to retrieve instruction.
			
			// Signals (as seen from HDL)
			bool accept;
			bool valid;
			bool running;

			//Inter-phase registers
			Instruction* s12;
			Instruction* s23;
			CoreOUT CO12;
			CoreOUT CO23;

			//settings
			bool verbose;

		public:
			Core(Instruction* p, bool dbg = false);
			void reset();

			bool isAccepted();
			bool isValid();
			bool isRunning();

			bool isStage2Ready();
			bool isStage3Ready();

			Instruction* getStage12();
			Instruction* getStage23();
			CoreOUT getCO12();
			CoreOUT getCO23();

			void stage1();
			// Multichar version
			void stage1(CoreOUT bufferOUT);

			void stage2();
			CoreOUT stage2(CoreOUT sCO12, Instruction* stage12, char currentChar);

			CoreOUT stage3(CoreOUT sCO23, Instruction* stage23);
	};

	class Manager {
		private:
			//Components
			Buffers* buffers;
			Core* core;

			// Manager signal
			unsigned short currentChar;
			unsigned short CC_ID;

			unsigned short HEAD;
			std::vector<bool> CCIDBitmap; 
			unsigned short size;

			//settings
			bool verbose;

		public:
			Manager(Buffers* b, Core* c, unsigned short W, bool dbg = false);
			void updateBitmap();
			unsigned short checkBitmap();

			int mod(int k, int n);

			bool runMultiChar(const char* input);
	};


	// Wrapper class that holds and inits all components.
	class SoftwareCICERO {
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
			SoftwareCICERO(unsigned short W = 1, bool dbg = false);

			void setProgram(const char* filename);
			bool isProgramSet();
			
			bool match(const char* input );
	};
}
#endif
