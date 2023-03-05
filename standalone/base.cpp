#include <iostream>
#include <queue>
#include <cstring>
#include <vector>

using namespace std;

#define INSTR_MEM_SIZE 512 //PC is 9bits
#define BITS_INSTR_TYPE 3
#define BITS_INSTR 16

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
		CoreOUT(){
			PC = 0;
			active = false;
		};

		CoreOUT(unsigned short PC, bool active){
			this->PC = PC;
			this->active = active;
		}


		unsigned short getPC(){return PC;}
		bool isActive(){return active;}
};

// Wrapper around the 16bit instruction for easy retrieval of type, data and easy printing.
class Instruction {
	private:
		unsigned short instr;
	public: 

		Instruction(){ instr = 0; }

		Instruction(unsigned short instruction){
			instr = instruction;
		}

		unsigned short getType(){ return instr >> (BITS_INSTR - BITS_INSTR_TYPE);};
		unsigned short getData(){ return instr % ( 1 << (BITS_INSTR - BITS_INSTR_TYPE));};
		
		
		void printType(int PC){
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

		void print(int pc){
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
};

// Container for all the buffers - permits to instantiate a variable number of buffers.
class Buffers {
	private:
		vector<queue<unsigned short>> buffers;
		int size; //2**W 
	public:
		Buffers(int n){ 
			size = n;
			buffers.reserve(n);
			for (int i = 0; i < n; i++){
				buffers.push_back(queue<unsigned short>());
			}
		}

		void flush(){
			for (int i = 0; i < size; i++){
				while (!buffers[i].empty()){
					buffers[i].pop();
				}
			}
		}

		bool isEmpty(unsigned short target){
			return buffers[target].empty();
		}

		// Expects to be told which is the buffer holding first character of sliding window.
		bool hasInstructionReady(unsigned short head){
			for (int i = 0; i < size -1 ; i++){ // Excludes the last buffer of the sliding window.

				if (!buffers[(head + i) % size].empty()) return true;
			}
			return false;
		}

		unsigned short getFirstNotEmpty(unsigned short head){
			for (int i = 0; i < size ; i++){ 	// Can return also the inactive one.
				if (!buffers[(head + i) % size].empty()) return (head+i) % size;
			}
			return size; //To be considered as all empty.
		}

		bool areAllEmpty(){
			bool areEmpty = true;
			for (int i = 0; i < size; i++){
				if (!buffers[i].empty()){
					areEmpty = false;
					break;
				}
			}
			return areEmpty;
		}

		unsigned short getPC(unsigned short target){
			unsigned short PC = buffers[target].front();
			return PC;
		}

		unsigned short popPC(unsigned short target){
			unsigned short PC = buffers[target].front();
			buffers[target].pop();
			return PC;
		}

		void pushTo(unsigned short target, unsigned short PC){

			if (target < size) 
				buffers[target].push(PC);
			else 
				fprintf(stderr, "[X] Pushing to non-existing buffer.\n");
		}
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
		Core(Instruction* p, bool dbg = false){
			program = p;
			accept = false;
			valid = false;
			running = true;
			s12 = NULL;
			s23 = NULL;
			PC12 = 0;
			verbose = dbg;
		}

		void reset(){
			accept = false;
			valid = false;
			running = true;
			s12 = NULL;
			s23 = NULL;
			PC12 = 0;

		}

		bool isAccepted(){return accept;}
		bool isValid(){return valid;}
		bool isRunning(){return running;}

		bool isStage2Ready(){
			if (s12 != NULL) return true;
			else return false;
		}

		bool isStage3Ready(){
			if (s23 != NULL && s23->getType() == SPLIT) return true;
			else return false;
		}

		Instruction* getStage12(){return s12;}
		Instruction* getStage23(){return s23;}
		unsigned short getPC12(){return PC12;}

		void stage1(){
			// Sets s12 to NULL if 
			s12 = NULL;
		}
		void stage1(unsigned short PC){
			s12 = &program[PC];
			PC12 = PC;
			if (verbose) { printf("\t(PC%d)(S1) ",PC); s12->printType(PC); printf("\n");}
		};

		void stage2(){
			s23 = NULL;
		}

		CoreOUT stage2(unsigned short sPC12, Instruction* stage12, char currentChar){
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
		};

		CoreOUT stage3(Instruction* s23){
			if (verbose) { printf("\t(S3)"); s23->printType(-1); printf("\n");}
			CoreOUT newPC = CoreOUT(s23->getData(), true);

			return newPC;
		};
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
		Manager(Buffers* b, Core* c, bool dbg = false){
			core = c;
			buffers = b;
			verbose = dbg;
		}

		bool runBase(const char* input){
			
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
				stage1Ready = !buffers->isEmpty(bufferSelection); //TODO: new verification of stage1ready to account for multichar
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
				// TODO: Test if this fails in the face of multiple SPLITs
				else if (buffers->isEmpty(bufferSelection)){ // Buffer ping pong.
					currentChar ++;
					if (verbose) printf("\t\tFIFO%x is empty, activating FIFO%x and moving to char %c\n", bufferSelection, !bufferSelection, input[currentChar]);
					bufferSelection = !bufferSelection;
				}
				
				CC++;
				// End the cycle AFTER having processed the '\0' (which can be consumed by an ACCEPT) or if no more instructions are left to be processed.
				if (strlen(input) < currentChar || buffers->areAllEmpty() && !core->isStage2Ready() && !core->isStage3Ready()) break;
				
			}

			return false;
		}
};

// Wrapper class that holds and inits all components.
class SoftwareCICERO {
	private:

		// Components
		Instruction program[INSTR_MEM_SIZE];
		Buffers buffers = Buffers(2);
		Core core = Core(&program[0]);
		Manager manager = Manager(&buffers, &core);

		// Settings
		bool verbose = true;
		bool hasProgram = false;

	public:
		SoftwareCICERO(bool dbg = false){

			hasProgram = false;
			buffers = Buffers(2);
			core = Core(&program[0], dbg);
			manager = Manager(&buffers, &core, dbg);
			verbose = dbg;
		}

		void setProgram(const char* filename){
			FILE* fp= fopen(filename,"r");
			unsigned short instr;
			int i;

			if (fp != NULL){
				
				if (verbose) printf("Reading program file: \n\n");

				for (i = 0; i < INSTR_MEM_SIZE && !feof(fp); i++){

					fscanf(fp,"%x",&instr);
					program[i] = Instruction(instr);
					fscanf(fp,"\n");

					//Prettyprint instruction
					if (verbose) program[i].print(i);
				}

				if ( i == INSTR_MEM_SIZE && !feof(fp)) {
					fprintf(stderr, "[X] Program memory exceeded. Only the first %x instructions were read.\n", INSTR_MEM_SIZE);
				}

				hasProgram = true;

			} else {
				fprintf(stderr, "[X] Could not open program file for reading.\n");
			}
		};
		
		bool match(const char* input ){

			if (!hasProgram){
				fprintf(stderr, "[X] No program is loaded to match the string against.\n");
				return NULL;
			} 

			return manager.runBase(input);
		}
};


int main(void){
		
	SoftwareCICERO CICERO = SoftwareCICERO(true);
	CICERO.setProgram("../runningex");


	if(CICERO.match("abaababd")) printf("regex %d 	, input %d (len: %d)	, match True\n",0,0,256);
	else printf("regex %d 	, input %d (len: %d)	, match False\n",0,0,256);

	return 0;
}
