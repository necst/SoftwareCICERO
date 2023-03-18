#ifndef CICEROBASE_H
#define CICEROBASE_H
#include <iostream>
#include <queue>
#include <cstring>
#include <vector>

#define INSTR_MEM_SIZE 512 //PC is 9bits
#define BITS_INSTR_TYPE 3
#define BITS_INSTR 16

namespace CiceroBase {
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
			bool active;
			unsigned short CC_ID;
		public:
			CoreOUT();
			CoreOUT(unsigned short PC, bool active);
			unsigned short getPC();
			bool isActive();
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
			int size; //2**W 
		public:
			Buffers();
			void flush();
			bool isEmpty(unsigned short target);
			// Expects to be told which is the buffer holding first character of sliding window.
			bool hasInstructionReady(unsigned short head);
			unsigned short getFirstNotEmpty(unsigned short head);
			bool areAllEmpty();
			unsigned short getPC(unsigned short target);
			unsigned short popPC(unsigned short target);
			void pushTo(unsigned short target, unsigned short PC);
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
			unsigned short PC12;

			//settings
			bool verbose;

		public:
			Core(Instruction* p, bool dbg);
			void reset();
			bool isAccepted();
			bool isValid();
			bool isRunning();

			bool isStage2Ready();
			bool isStage3Ready();
			Instruction* getStage12();
			Instruction* getStage23();
			unsigned short getPC12();

			void stage1();
			void stage1(unsigned short PC);

			void stage2();

			CoreOUT stage2(unsigned short sPC12, Instruction* stage12, char currentChar);

			CoreOUT stage3(Instruction* s23);
	};

	class Manager {
		private:
			//Components
			Buffers* buffers;
			Core* core;

			// Manager signal
			unsigned short currentChar;
			unsigned short CC_ID;

			unsigned short bufferSelection;

			//settings
			bool verbose;

		public:
			Manager(Buffers* b, Core* c, bool dbg = false);
			bool runBase(const char* input);
	};

	// Wrapper class that holds and inits all components.
	class SoftwareCICERO {
		private:

			// Components
			Instruction program[INSTR_MEM_SIZE];
			Buffers buffers;
			Core core;
			Manager manager;

			// Settings
			bool verbose;
			bool hasProgram;

		public:
			SoftwareCICERO(bool dbg = false);

			void setProgram(const char* filename);
			bool isProgramSet();
			
			bool match(const char* input);
	};
}
#endif
