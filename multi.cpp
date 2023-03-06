#include <iostream>
#include <queue>
#include <cstring>
#include <vector>
#include <cmath>

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
		unsigned short CC_ID;
	public:
		CoreOUT(){
			PC = 0;
			CC_ID = 0;
		};

		CoreOUT(unsigned short PC){
			this->PC = PC;
		}

		CoreOUT(unsigned short PC,unsigned short CC_ID){
			this->PC = PC;
			this->CC_ID = CC_ID;
		}

		unsigned short getPC(){return PC;}
		unsigned short getCC_ID(){return CC_ID;}
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

				void printQueue(unsigned short CC_ID){
			if (!buffers[CC_ID].empty()){
				printf("Queue %d  next item: %d, last:%d", CC_ID, buffers[CC_ID].front(),  buffers[CC_ID].back());
				printf("\n");
			}
			else printf("Queue %d empty\n", CC_ID);
		}

		bool isEmpty(unsigned short CC_ID){
			return buffers[(CC_ID)%size].empty();
		}


		// Expects to be told which is the buffer holding first character of sliding window.
		bool hasInstructionReady(unsigned short HEAD){
			for (int i = 0; i < size -1 ; i++){ // Excludes the last buffer of the sliding window.
				if (!buffers[(HEAD + i) % size].empty()) {
					return true;
				}
			}
			return false;
		}

		unsigned short getFirstNotEmpty(unsigned short HEAD){
			for (int i = 0; i < size - 1 ; i++){ 	// Cannot return the inactive one.
													// TODO: head used only here in round check. other uses round.
				//printf("Find first instruction CC_ID:%d,%d",i+HEAD, buffers[(i + HEAD)%size].front());

				if (!buffers[(HEAD + i) % size].empty()) {
					return (HEAD + i) %size;
				}
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

		CoreOUT getPC(unsigned short CC_ID){
			CoreOUT PC = CoreOUT(buffers[(CC_ID)%size].front(), CC_ID);
			return PC;
		}

		CoreOUT popPC(unsigned short CC_ID){
			CoreOUT PC = CoreOUT(buffers[(CC_ID)%size].front(), CC_ID);
			buffers[(CC_ID)%size].pop();
			return PC;
		}

		void pushTo(unsigned short CC_ID, unsigned short PC){

			if (CC_ID < size){ 
				buffers[(CC_ID)%size].push(PC);
			}
			else 
				fprintf(stderr, "[X] Pushing to non-existing buffer %d.\n", CC_ID);
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
		CoreOUT CO12;
		CoreOUT CO23;

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
			CO12 = CoreOUT();
			CO23 = CoreOUT();
			verbose = dbg;
		}

		void reset(){
			accept = false;
			valid = false;
			running = true;
			s12 = NULL;
			s23 = NULL;
			CO12 = CoreOUT();
			CO23 = CoreOUT();

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
		CoreOUT getCO12(){return CO12;}
		CoreOUT getCO23(){return CO23;}

		void stage1(){
			// Sets s12 to NULL if 
			s12 = NULL;
		}

		// Multichar version
		void stage1(CoreOUT bufferOUT){
			s12 = &program[bufferOUT.getPC()];
			CO12 = bufferOUT;
			if (verbose) { printf("\t(PC%d)(CC_ID%d)(S1) ",bufferOUT.getPC(),bufferOUT.getCC_ID()); s12->printType(bufferOUT.getPC()); printf("\n");}
		};

		void stage2(){
			s23 = NULL;
		}

		CoreOUT stage2(CoreOUT sCO12, Instruction* stage12, char currentChar){
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

		CoreOUT stage3(CoreOUT sCO23, Instruction* stage23){
			//TODO: add a normal call without hardcoded -1
			if (verbose) { printf("\t(PC%d)(CC_ID%d)(S3)", sCO23.getPC(), sCO23.getCC_ID()); stage23->printType(sCO23.getPC()+1); printf("\n");}
			CoreOUT newPC = CoreOUT(stage23->getData(), sCO23.getCC_ID());

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

		unsigned short HEAD;
		vector<bool> CCIDBitmap; 
		unsigned short size;

		//settings
		bool verbose;

	public:
		Manager(Buffers* b, Core* c, unsigned short W, bool dbg = false){
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

		void updateBitmap(){
			//TODO: push or pull? is it the manager checking? OR core/buffers notifying? Seems notifying from ith active diagram. 
			//TODO: on push to buffer, pop from buffer, end of clock cycle?
			//Check buffers
			for (unsigned short i = 0; i < CCIDBitmap.size(); i++){
				CCIDBitmap[i] = !buffers->isEmpty(i) | ((core->getCO12().getCC_ID() == i)&core->getStage12() != NULL) | ((core->getCO23().getCC_ID() == i)&core->getStage23() != NULL)  ;

			/*	printf("Core12: %d\n", core->getCO12().getCC_ID() );
				printf("Core23: %d\n", core->getCO23().getCC_ID() );
				printf("CC_ID: %d conditions %d,%d,%d -> result: %d\n",i, !buffers->isEmpty(i), ((core->getCO12().getCC_ID() == i)&core->getStage12() != NULL) , ((core->getCO23().getCC_ID() == i)&core->getStage23() != NULL), (bool)CCIDBitmap[i]) ;
				*/
			}
			
		}

		unsigned short checkBitmap(){
			
			unsigned short slide = 0;
			for (unsigned short i = 0; i < CCIDBitmap.size(); i++){
				if (CCIDBitmap[(HEAD+i)%size] == 0) slide += 1;
				else break;
			}
			return slide;
			
		}
		int mod(int k, int n) {
			return ((k %= n) < 0) ? k+n : k;
		}


		bool runMultiChar(const char* input){
			
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
//TODO: reset bitmap
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
						if (verbose) printf("\t\tPushing PC%d to FIFO%d (beforemod: %d)\n", newPC.getPC(), newPC.getCC_ID()%size, newPC.getCC_ID()); 
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
		Manager manager = Manager(&buffers, &core, 2);

		// Settings
		bool verbose = true;
		bool hasProgram = false;

	public:
		SoftwareCICERO(unsigned short W = 1, bool dbg = false){

			if (W == 0) W = 1;

			buffers = Buffers(W+1);

			hasProgram = false;
			core = Core(&program[0], dbg);
			manager = Manager(&buffers, &core, W+1, dbg);
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

					// Pretty print instructions
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

			//TODO: checks on input string not being malformed.

			if (!hasProgram){
				fprintf(stderr, "[X] No program is loaded to match the string against.\n");
				return NULL;
			} 

			return manager.runMultiChar(input);
		}
};


int main(void){
		
	SoftwareCICERO CICERO = SoftwareCICERO(2,true);
	CICERO.setProgram("./runningex");


	if(CICERO.match("abaababd")) printf("regex %d 	, input %d (len: %d)	, match True\n",0,0,256);
	else printf("regex %d 	, input %d (len: %d)	, match False\n",0,0,256);


	return 0;
}

