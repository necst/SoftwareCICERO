#include <iostream>
#include <queue>
#include <cstring>
#include <vector>
#include <cmath>
#include "CiceroMulti.h"

namespace CiceroMulti {
	using namespace std;

	CoreOUT::CoreOUT(){
		PC = 0;
		CC_ID = 0;
	};

	CoreOUT::CoreOUT(unsigned short PC){
		this->PC = PC;
	}

	CoreOUT::CoreOUT(unsigned short PC,unsigned short CC_ID){
		this->PC = PC;
		this->CC_ID = CC_ID;
	}

	unsigned short CoreOUT::getPC(){return PC;}
	unsigned short CoreOUT::getCC_ID(){return CC_ID;}

// Wrapper around the 16bit instruction for easy retrieval of type, data and easy printing.
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

	};

// Container for all the buffers - permits to instantiate a variable number of buffers.
	Buffers::Buffers(int n){ 
		size = n;
		buffers.reserve(n);
		for (int i = 0; i < n; i++){
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

	bool Buffers::isEmpty(unsigned short CC_ID){
		return buffers[(CC_ID)%size].empty();
	}


	// Expects to be told which is the buffer holding first character of sliding window.
	bool Buffers::hasInstructionReady(unsigned short HEAD){
		for (int i = 0; i < size -1 ; i++){ // Excludes the last buffer of the sliding window.
			if (!buffers[(HEAD + i) % size].empty()) {
				return true;
			}
		}
		return false;
	}

	unsigned short Buffers::getFirstNotEmpty(unsigned short HEAD){
		for (int i = 0; i < size - 1 ; i++){ 	// Cannot return the inactive one.

			if (!buffers[(HEAD + i) % size].empty()) {
				return (HEAD + i) %size;
			}
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

	CoreOUT Buffers::getPC(unsigned short CC_ID){
		CoreOUT PC = CoreOUT(buffers[(CC_ID)%size].front(), CC_ID);
		return PC;
	}

	CoreOUT Buffers::popPC(unsigned short CC_ID){
		CoreOUT PC = CoreOUT(buffers[(CC_ID)%size].front(), CC_ID);
		buffers[(CC_ID)%size].pop();
		return PC;
	}

	void Buffers::pushTo(unsigned short CC_ID, unsigned short PC){

		if (CC_ID < size){ 
			buffers[(CC_ID)%size].push(PC);
		}
		else 
			fprintf(stderr, "[X] Pushing to non-existing buffer %d.\n", CC_ID);
	}



	Core::Core(Instruction* p, bool dbg){
		program = p;
		accept = false;
		valid = false;
		running = true;
		s12 = NULL;
		s23 = NULL;
		CO12 = CoreOUT();
		CO23 = CoreOUT();
		verbose = dbg;
	}

	void Core::reset(){
		accept = false;
		valid = false;
		running = true;
		s12 = NULL;
		s23 = NULL;
		CO12 = CoreOUT();
		CO23 = CoreOUT();

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
	CoreOUT Core::getCO12(){return CO12;}
	CoreOUT Core::getCO23(){return CO23;}

	void Core::stage1(){
		// Sets s12 to NULL if 
		s12 = NULL;
	}

	// Multichar version
	void Core::stage1(CoreOUT bufferOUT){
		s12 = &program[bufferOUT.getPC()];
		CO12 = bufferOUT;
		if (verbose) { printf("\t(PC%d)(CC_ID%d)(S1) ",bufferOUT.getPC(),bufferOUT.getCC_ID()); s12->printType(bufferOUT.getPC()); printf("\n");}
	};

	void Core::stage2(){
		s23 = NULL;
	}

	CoreOUT Core::stage2(CoreOUT sCO12, Instruction* stage12, char currentChar){
		// Stage 2: get next PC and handle ACCEPT
		if (stage12->getType() == 1) s23 = stage12;
		else s23 = NULL;
		CO23 = sCO12;
		CoreOUT newPC;
		running = true;
		accept = false;
		valid = false;

		if (verbose) { printf("\t(PC%d)(CC_ID%d)(S2) ", sCO12.getPC(),sCO12.getCC_ID()); stage12->printType(sCO12.getPC());printf("\n");}

		switch(stage12->getType()){

			case ACCEPT:
				if (currentChar == '\0') accept = true;
				newPC = CoreOUT();
				break;

			case SPLIT:
				valid = true;
				newPC = CoreOUT( sCO12.getPC()+1 , sCO12.getCC_ID());
				break;

			case MATCH:
				if (char(stage12->getData())==currentChar){
					valid = true;
					newPC = CoreOUT(sCO12.getPC()+1, sCO12.getCC_ID()+1);
					if (verbose) { printf("\t\tCharacters matched: input %c to %c\n", currentChar, char(stage12->getData()));}
				} else {
					if (verbose) { printf("\t\tCharacters not matched: input %c to %c\n", currentChar, char(stage12->getData()));}
					newPC = CoreOUT();
				}
				break;

			case JMP:
				valid = true;
				newPC = (CoreOUT(stage12->getData(), sCO12.getCC_ID()));
				break;

			case END_WITHOUT_ACCEPTING:
				running = false;
				newPC = CoreOUT();
				break;

			case MATCH_ANY:
				valid = true;
				if (verbose) { printf("\t\tCharacters matched: input %c to ANY\n", currentChar );}
				newPC = (CoreOUT(sCO12.getPC()+1, (sCO12.getCC_ID()+1)));
				break;

			case ACCEPT_PARTIAL:
				accept = true;
				newPC = CoreOUT();
				break;

			case NOT_MATCH:
				if (char(stage12->getData())!=currentChar){
					valid = true;
					newPC = CoreOUT( sCO12.getPC()+1, sCO12.getCC_ID());
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
	};

	CoreOUT Core::stage3(CoreOUT sCO23, Instruction* stage23){
		if (verbose) { printf("\t(PC%d)(CC_ID%d)(S3)", sCO23.getPC(), sCO23.getCC_ID()); stage23->printType(sCO23.getPC()+1); printf("\n");}
		CoreOUT newPC = CoreOUT(stage23->getData(), sCO23.getCC_ID());

		return newPC;
	};

	Manager::Manager(Buffers* b, Core* c, unsigned short W, bool dbg){
		core = c;
		buffers = b;
		verbose = dbg;
		size = W;
		HEAD = 0;
		CCIDBitmap.reserve(W);
		for (int i = 0; i < W; i++){
			CCIDBitmap.push_back(false);
		}


	}

	void Manager::updateBitmap(){
		//Check buffers
		for (unsigned short i = 0; i < CCIDBitmap.size(); i++){
			CCIDBitmap[i] = (!buffers->isEmpty(i)) | ((core->getCO12().getCC_ID() == i) & (core->getStage12() != NULL)) | ((core->getCO23().getCC_ID() == i) & (core->getStage23() != NULL))  ;

		}
		
	}

	unsigned short Manager::checkBitmap(){
		
		unsigned short slide = 0;
		for (unsigned short i = 0; i < CCIDBitmap.size(); i++){
			if (CCIDBitmap[(HEAD+i)%size] == 0) slide += 1;
			else break;
		}
		return slide;
		
	}

	int Manager::mod(int k, int n) {
		return ((k %= n) < 0) ? k+n : k;
	}


	bool Manager::runMultiChar(const char* input){
		
		// Signals handled by manager
		currentChar = 0; // In MultiChar it refers to first character in sliding window.
		HEAD = 0;


		// Local variables to emulate clock cycles.
		int CC = 0;
		CoreOUT newPC;
		bool stage2Ready = false;
		bool stage1Ready = false;
		bool stage3Ready = false;
		Instruction* s12 = NULL;
		Instruction* s23 = NULL;
		CoreOUT CO12 = CoreOUT();
		CoreOUT CO23 = CoreOUT();

		// Reset to known empty state.
		core->reset();
		buffers->flush();
		/*for (int i = 0; i < size; i++){
			CCIDBitmap.push_back(false);
		}*/
		// Load first instruction PC.
		buffers->pushTo(0, 0);
		CCIDBitmap[0] = true;

		if (verbose) printf("\nInitiating match of string %s\n", input);

		// Simulate clock cycle
		while (!core->isAccepted() && core->isRunning()){
		
			if (verbose) printf("[CC%d] Window first character: %c\n", CC, input[currentChar]);

			/* READ
				* To simulate the stages being concurrent, all values used by stages 2 and 3 
				* must be read at the start of the cycle, emulating values being read on rise. */

			// Check at the start whether each stage meets the conditions for being executed.
			stage1Ready = buffers->hasInstructionReady(HEAD);
			stage2Ready = core->isStage2Ready(); 
			stage3Ready = core->isStage3Ready(); 

			
			if (verbose) printf("\tStages deemed ready: %x, %x, %x\n", stage1Ready, stage2Ready, stage3Ready);
			// Save the inter-stage registers for use.
			s12 = core->getStage12();
			s23 = core->getStage23();
			CO12 = core-> getCO12();
			CO23 = core-> getCO23();

			/* EXEC */

			// Stage 1: retrieve newPC from active buffer and load instruction. 
			if (stage1Ready) core->stage1(buffers->getPC(buffers->getFirstNotEmpty(HEAD)));
			else {core->stage1();} //Empty run to clear signals.

			// Stage 2: sets valid, running and accept signals. 
			if (stage2Ready) {
				//printf("HEAD: %d, HEADsize:%d, CC_ID%d, currchar %d %c\n", HEAD, HEAD%size, CO12.getCC_ID(), currentChar, input[currentChar]);

				newPC = core->stage2(CO12, s12, input[currentChar+mod((CO12.getCC_ID()-HEAD),(size))]);

				// Handle the returned value, if it's a valid one. 
				if (core->isValid()){
					if (verbose) printf("\t\tPushing PC%d to FIFO%d\n", newPC.getPC(), newPC.getCC_ID()%size); 
					// Push to correct buffer
					buffers->pushTo(newPC.getCC_ID()%size, newPC.getPC()); 
				// Invalid values that must be handled are returned by ACCEPT, ACCEPT_PARTIAL and END_WITHOUT_ACCEPTING.
				// Apart from these, the only way for a computation to end is by reaching end of string without ACCEPT.
				} else if (core->isAccepted()){
					return true;
				} else if (!core->isRunning()){
					return false;
				} 

			} else { core->stage2(); } //Empty run to clear signals

			// Stage 3: Only executed by a SPLIT instruction
			if (stage3Ready){

				newPC = core->stage3(CO23,s23);

				if (verbose) printf("\t\tPushing PC%d to FIFO%x\n", newPC.getPC(), newPC.getCC_ID()%size); 
				// Push to correct buffer
				buffers->pushTo(newPC.getCC_ID()%size, newPC.getPC()); 
			}

			/* WRITEBACK 
				* Values should only be modified at the end of the clock cycle */

			// Only if stage 1 was executed, consume the instruction that was loaded from the buffer.
			// Otherwise, it would risk consuming a value added by stage 2 in the same cycle.
			if (stage1Ready){
				if (verbose) printf("\t\tConsumed PC%d from FIFO%d, relating to character %c\n", core->getCO12().getPC(), core->getCO12().getCC_ID(), input[currentChar+mod((CO12.getCC_ID()-HEAD),(size))]);
				
				buffers->popPC(core->getCO12().getCC_ID());		
				if (verbose) printf("\t\tNext PC from FIFO%d: %d\n", buffers->getFirstNotEmpty(HEAD), buffers->getPC(buffers->getFirstNotEmpty(HEAD)).getPC());
			} 

			updateBitmap();
			

			if (checkBitmap() != 0){ // Conditions for sliding the window.
										
				if (verbose) printf("\t\t%x Threads are inactive, sliding window. New first char in window: %c\n", checkBitmap(), input[currentChar]);
				currentChar += checkBitmap(); // Move the window + i
				HEAD = (HEAD + checkBitmap())%size;
			}
			
			CC++;
			// End the cycle AFTER having processed the '\0' (which can be consumed by an ACCEPT) or if no more instructions are left to be processed.
			if (strlen(input) < currentChar || (buffers->areAllEmpty() && !core->isStage2Ready() && !core->isStage3Ready())) break;
			
		}

		return false;
	}


// Wrapper class that holds and inits all components.
	SoftwareCICERO::SoftwareCICERO(unsigned short W, bool dbg){

		if (W == 0) W = 1;

		buffers = Buffers(W+1);

		hasProgram = false;
		core = Core(&program[0], dbg);
		manager = Manager(&buffers, &core, W+1, dbg);
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

				// Pretty print instructions
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

	bool SoftwareCICERO::SoftwareCICERO::isProgramSet(){

			return hasProgram;
	}

	bool SoftwareCICERO::match(const char* input ){


		if (!hasProgram){
			fprintf(stderr, "[X] No program is loaded to match the string against.\n");
			return NULL;
		}

		return manager.runMultiChar(input);
	}
}



