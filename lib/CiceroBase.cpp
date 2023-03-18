#include <iostream>
#include <queue>
#include <cstring>
#include <vector>
#include "CiceroBase.h"

namespace CiceroBase {
	using namespace std;

	// Models the output of the Core, which is the new PC plus the bit indicating whether 
	// the returned instruction refers to the active character or not.
	CoreOUT::CoreOUT(){
				PC = 0;
				active = false;
	}

	CoreOUT::CoreOUT(unsigned short PC, bool active){
				this->PC = PC;
				this->active = active;
	}


	unsigned short CoreOUT::getPC(){return PC;}
	bool CoreOUT::isActive(){return active;}

	// Wrapper around instruction

	Instruction::Instruction(){ instr = 0; }

	Instruction::Instruction(unsigned short instruction){
		instr = instruction;
	}

	unsigned short Instruction::getType(){ return instr >> (BITS_INSTR - BITS_INSTR_TYPE);};
	unsigned short Instruction::getData(){ return instr % ( 1 << (BITS_INSTR - BITS_INSTR_TYPE));};


	void Instruction::printType(int PC){
		switch (this->getType())
		{
		case 0:
			printf("ACCEPT");
			break;
		case 1:
			printf("SPLIT{%d,%d}", PC+1, this->getData());
			break;
		case 2:
			printf("MATCH(%c)", this->getData());
			break;
		case 3:
			printf("JMP(%d)", this->getData());
			break;
		case 4:
			printf("END_WITHOUT_ACCEPTING");
			break;
		case 5:
			printf("MATCH_ANY");
			break;
		case 6:
			printf("ACCEPT_PARTIAL");
			break;
		case 7:
			printf("NOT_MATCH(%c)", this->getData());
			break;
		default:
			printf("UNKNOWN");
			break;
		}
	}

	void Instruction::print(int pc){
		printf("%03d: %x \\\\ ", pc, instr);
		switch (this->getType())
		{
		case 0:
			printf("ACCEPT\n");
			break;
		case 1:
			printf("SPLIT\t {%d,%d} \n", pc+1, this->getData());
			break;
		case 2:
			printf("MATCH\t char %c\n",  this->getData());
			break;
		case 3:
			printf("JMP to \t %d \n",this->getData());
			break;
		case 4:
			printf("END_WITHOUT_ACCEPTING\n");
			break;
		case 5:
			printf("MATCH_ANY\n");
			break;
		case 6:
			printf("ACCEPT_PARTIAL\n");
			break;
		case 7:
			printf("NOT_MATCH\t char %c\n", this->getData());
			break;
		default:
			printf("UNKNOWN %d\t data %d\n",this->getType(), this->getData());
			break;
		}

	}

	// Container for all the buffers - permits to instantiate a variable number of buffers.
	Buffers::Buffers(){ 
		size = 2;
		buffers.reserve(2);
		for (int i = 0; i < 2; i++){
			buffers.push_back(queue<unsigned short>());
		}
	}

	void Buffers::flush(){
		for (int i = 0; i < size; i++){
			while (!buffers[i].empty()){
				buffers[i].pop();
			}
		}
	}

	bool Buffers::isEmpty(unsigned short target){
		return buffers[target].empty();
	}

	// Expects to be told which is the buffer holding first character of sliding window.
	bool Buffers::hasInstructionReady(unsigned short head){
		for (int i = 0; i < size -1 ; i++){ // Excludes the last buffer of the sliding window.

			if (!buffers[(head + i) % size].empty()) return true;
		}
		return false;
	}

	unsigned short Buffers::getFirstNotEmpty(unsigned short head){
		for (int i = 0; i < size ; i++){ 	// Can return also the inactive one.
			if (!buffers[(head + i) % size].empty()) return (head+i) % size;
		}
		return size; //To be considered as all empty.
	}

	bool Buffers::areAllEmpty(){
		bool areEmpty = true;
		for (int i = 0; i < size; i++){
			if (!buffers[i].empty()){
				areEmpty = false;
				break;
			}
		}
		return areEmpty;
	}

	unsigned short Buffers::getPC(unsigned short target){
		unsigned short PC = buffers[target].front();
		return PC;
	}

	unsigned short Buffers::popPC(unsigned short target){
		unsigned short PC = buffers[target].front();
		buffers[target].pop();
		return PC;
	}

	void Buffers::pushTo(unsigned short target, unsigned short PC){

		if (target < size) 
			buffers[target].push(PC);
		else 
			fprintf(stderr, "[X] Pushing to non-existing buffer.\n");
	}




	Core::Core(Instruction* p, bool dbg = false){
		program = p;
		accept = false;
		valid = false;
		running = true;
		s12 = NULL;
		s23 = NULL;
		PC12 = 0;
		verbose = dbg;
	}

	void Core::reset(){
		accept = false;
		valid = false;
		running = true;
		s12 = NULL;
		s23 = NULL;
		PC12 = 0;

	}

	bool Core::isAccepted(){return accept;}
	bool Core::isValid(){return valid;}
	bool Core::isRunning(){return running;}

	bool Core::isStage2Ready(){
		if (s12 != NULL) return true;
		else return false;
	}

	bool Core::isStage3Ready(){
		if (s23 != NULL && s23->getType() == SPLIT) return true;
		else return false;
	}

	Instruction* Core::getStage12(){return s12;}
	Instruction* Core::getStage23(){return s23;}
	unsigned short Core::getPC12(){return PC12;}

	void Core::stage1(){
		// Sets s12 to NULL if 
		s12 = NULL;
	}
	void Core::stage1(unsigned short PC){
		s12 = &program[PC];
		PC12 = PC;
		if (verbose) { printf("\t(PC%d)(S1) ",PC); s12->printType(PC); printf("\n");}
	};

	void Core::stage2(){
		s23 = NULL;
	}

	CoreOUT Core::stage2(unsigned short sPC12, Instruction* stage12, char currentChar){
		// Stage 2: get next PC and handle ACCEPT
		s23 = stage12;
		CoreOUT newPC;
		running = true;
		accept = false;
		valid = false;

		if (verbose) { printf("\t(PC%d)(S2) ", sPC12); stage12->printType(sPC12);printf("\n");}

		switch(stage12->getType()){

			case ACCEPT:
				if (currentChar == '\0') accept = true;
				newPC = CoreOUT();
				break;

			case SPLIT:
				valid = true;
				newPC = CoreOUT( sPC12+1 , true);
				break;

			case MATCH:
				if (char(stage12->getData())==currentChar){
					valid = true;
					newPC = CoreOUT( sPC12+1, false);
					if (verbose) { printf("\t\tCharacters matched: input %c to %c\n", currentChar, char(stage12->getData()));}
				} else {
					if (verbose) { printf("\t\tCharacters not matched: input %c to %c\n", currentChar, char(stage12->getData()));}
					newPC = CoreOUT();
				}
				break;

			case JMP:
				valid = true;
				newPC = (CoreOUT(stage12->getData(), true));
				break;

			case END_WITHOUT_ACCEPTING:
				running = false;
				newPC = CoreOUT();
				break;

			case MATCH_ANY:
				valid = true;
				if (verbose) { printf("\t\tCharacters matched: input %c to ANY\n", currentChar );}
				newPC = (CoreOUT(sPC12+1, false));
				break;

			case ACCEPT_PARTIAL:
				accept = true;
				newPC = CoreOUT();
				break;

			case NOT_MATCH:
				if (char(stage12->getData())!=currentChar){
					valid = true;
					newPC = CoreOUT( sPC12+1, true);
				} else {
					newPC = CoreOUT();
				}
				break;

			default:
				fprintf(stderr, "[X] Malformed instruction found.");
				newPC =  CoreOUT();
				break;
		}
		return newPC;
	}

	CoreOUT Core::stage3(Instruction* s23){
		if (verbose) { printf("\t(S3)"); s23->printType(-1); printf("\n");}
		CoreOUT newPC = CoreOUT(s23->getData(), true);

		return newPC;
	}

	Manager::Manager(Buffers* b, Core* c, bool dbg){
		core = c;
		buffers = b;
		verbose = dbg;
	}

	bool Manager::runBase(const char* input){
		
		// Signals handled by manager
		currentChar = 0;
		bufferSelection = 0;

		// Local variables to emulate clock cycles.
		int CC = 0;
		CoreOUT newPC;
		bool stage2Ready = false;
		bool stage1Ready = false;
		bool stage3Ready = false;
		Instruction* s12 = NULL;
		Instruction* s23 = NULL;
		unsigned short PC12;

		// Reset to known empty state.
		core->reset();
		buffers->flush();

		// Load first instruction PC.
		buffers->pushTo(bufferSelection, 0);

		if (verbose) printf("\nInitiating match of string %s\n", input);

		// Simulate clock cycle
		while (!core->isAccepted() && core->isRunning()){
		

			if (verbose) printf("[CC%d] Char:%c, Active buffer: FIFO%d\n", CC, input[currentChar], bufferSelection);

			/* To simulate the stages being concurrent, all values used by stages 2 and 3 
				* must be read at the start of the cycle, emulating values being read on rise. */

			// Check at the start whether each stage meets the conditions for being executed.
			stage1Ready = !buffers->isEmpty(bufferSelection); 
			stage2Ready = core->isStage2Ready(); 
			stage3Ready = core->isStage3Ready(); 

			// Save the inter-stage registers for use.
			s12 = core->getStage12();
			s23 = core->getStage23();
			PC12 = core->getPC12();

			/* EXEC */

			// Stage 1: retrieve newPC from active buffer and load instruction.
			if (stage1Ready) core->stage1(buffers->getPC(bufferSelection));
			else {core->stage1();}

			// Stage 2: sets valid, running and accept signals. 
			if (stage2Ready) {
				newPC = core->stage2(PC12, s12, input[currentChar]);

				// Handle the returned value, if it's a valid one. 
				if (core->isValid()){
					// Push to correct buffer
					if (newPC.isActive()){
						if (verbose) printf("\t\tPushing PC%d to active FIFO%x\n", newPC.getPC(), bufferSelection); //CC_ID + 1
						buffers->pushTo(bufferSelection, newPC.getPC());
					} else {
						if (verbose) printf("\t\tPushing PC%d to inactive FIFO%x\n", newPC.getPC(), !bufferSelection);
						buffers->pushTo(!bufferSelection, newPC.getPC());
					}
				// Invalid values that must be handled are returned by ACCEPT, ACCEPT_PARTIAL and END_WITHOUT_ACCEPTING.
				// Apart from these, the only way for a computation to end is by reaching end of string without ACCEPT.
				} else if (core->isAccepted()){
					return true;
				} else if (!core->isRunning()){
					return false;
				} 

			} else { core->stage2(); }

			// Stage 3: Only executed by a SPLIT instruction
			if (stage3Ready){

				newPC = core->stage3(s23);
				
				// Push to correct buffer
				if (newPC.isActive()){
					if (verbose) printf("\t\tPushing PC%d to active FIFO%x\n", newPC.getPC(), bufferSelection);
					buffers->pushTo(bufferSelection, newPC.getPC());
				} else {
					if (verbose) printf("\t\tPushing PC%d to inactive FIFO%x\n", newPC.getPC(), !bufferSelection);
					buffers->pushTo(!bufferSelection, newPC.getPC());
				}
			}

			/* WRITEBACK 
				* Values should only be modified at the end of the clock cycle */

			// Only if stage 1 was executed, consume the instruction that was loaded from the buffer.
			// Otherwise, it would risk consuming a value added by stage 2 in the same cycle.
			if (stage1Ready){
				
				if (verbose) printf("\t\tConsumed PC%d from active FIFO%x. Continue operating on char %c\n", buffers->getPC(bufferSelection), bufferSelection, input[currentChar]);
				buffers->popPC(bufferSelection);				
			} 
			// If buffer was empty both now and at the start of the cycle, it's safe to deem it empty and switch to handling a new character
			else if (buffers->isEmpty(bufferSelection)){ // Buffer ping pong.
				currentChar ++;
				if (verbose) printf("\t\tFIFO%x is empty, activating FIFO%x and moving to char %c\n", bufferSelection, !bufferSelection, input[currentChar]);
				bufferSelection = !bufferSelection;
			}
			
			CC++;
			// End the cycle AFTER having processed the '\0' (which can be consumed by an ACCEPT) or if no more instructions are left to be processed.
			if (strlen(input) < currentChar || (buffers->areAllEmpty() && !core->isStage2Ready() && !core->isStage3Ready())) break;
			
		}

		return false;
	}


	// Wrapper class that holds and inits all components.
	SoftwareCICERO::SoftwareCICERO(bool dbg): core(Core(&program[0], dbg)), manager(Manager(&buffers, &core, dbg)) {

		hasProgram = false;
		verbose = dbg;
	}

	void SoftwareCICERO::setProgram(const char* filename){
		FILE* fp= fopen(filename,"r");
		unsigned short instr;
		int i;

		if (fp != NULL){
			
			if (verbose) printf("Reading program file: \n\n");

			for (i = 0; i < INSTR_MEM_SIZE && !feof(fp); i++){

				fscanf(fp,"%hx",&instr);
				program[i] = Instruction(instr);
				fscanf(fp,"\n");

				//Prettyprint instruction
				if (verbose) program[i].print(i);
			}

			if ( i == INSTR_MEM_SIZE && !feof(fp)) {
				fprintf(stderr, "[X] Program memory exceeded. Only the first %x instructions were read.\n", INSTR_MEM_SIZE);
			}

			hasProgram = true;
			fclose(fp);

		} else {
			hasProgram = false;
			fprintf(stderr, "[X] Could not open program file %s for reading.\n", filename);
		}
	};

	bool SoftwareCICERO::isProgramSet(){

		return hasProgram;
	}

	bool SoftwareCICERO::match(const char* input ){

		if (!hasProgram){
			fprintf(stderr, "[X] No program is loaded to match the string against.\n");
			return false;
		} 

		return manager.runBase(input);
	}
}
