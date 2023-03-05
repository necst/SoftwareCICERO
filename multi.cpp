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
			CC_ID = 0; //TODO: check this is coherent with usage
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
		vector<queue<CoreOUT>> buffers;
		int HEAD;
		int size; //2**W 
	public:
		Buffers(int n){ 
			size = n;
			HEAD = 0;
			buffers.reserve(n);
			for (int i = 0; i < n; i++){
				buffers.push_back(queue<CoreOUT>());
			}
		}

		void flush(){
			for (int i = 0; i < size; i++){
				while (!buffers[i].empty()){
					buffers[i].pop();
				}
			}
			HEAD = 0;
		}

		void printQueue(unsigned short CC_ID){
			queue<CoreOUT> q = buffers[(HEAD + CC_ID)%size];
			if (!q.empty()){
				//printf("Queue %d (HEAD: %d) next item: %d, last:%d", CC_ID, HEAD, q.front().getPC(),  q.back().getPC());
				//printf("\n");
			}
			//else printf("Queue %d empty\n", CC_ID);
		}

		bool isEmpty(unsigned short CC_ID){
			return buffers[(HEAD + CC_ID)%size].empty();
		}

		void slide(unsigned short slide){

			HEAD = (HEAD + slide)% size;
		}
		 

		// Expects to be told which is the buffer holding first character of sliding window.
		bool hasInstructionReady(){
			for (int i = 0; i < size -1 ; i++){ // Excludes the last buffer of the sliding window.
				if (!buffers[(HEAD + i) % size].empty()) {
					return true;
				}
			}
			return false;
		}

		unsigned short getFirstNotEmpty(){
			for (int i = 0; i < size - 1 ; i++){ 	// Cannot return the inactive one.
				if (!buffers[(HEAD + i) % size].empty()) {
					//printf("FIFO%d (CC_ID %d) not empty, content:%d, back: %d\n", HEAD+i, i, buffers[(HEAD + i) % size].front().getPC(),  buffers[(HEAD + i) % size].back().getPC());
					return i;
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
			//CoreOUT PC = buffers[(HEAD + CC_ID)%size].front();
			CoreOUT PC = CoreOUT(buffers[(HEAD + CC_ID)%size].front().getPC(), CC_ID);
			return PC;
		}

		CoreOUT popPC(unsigned short CC_ID){
			//CoreOUT PC = buffers[(HEAD+CC_ID)%size].front();
			CoreOUT PC = CoreOUT(buffers[(HEAD + CC_ID)%size].front().getPC(), CC_ID);
			buffers[(HEAD+CC_ID)%size].pop();
			return PC;
		}

		void pushTo(unsigned short CC_ID, CoreOUT PC){

			if (CC_ID < size){ 
				buffers[(CC_ID + HEAD)%size].push(PC);
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
			if (verbose) { printf("\t(PC%d)(S1) ",bufferOUT.getPC()); s12->printType(bufferOUT.getPC()); printf("\n");}
		};

		void stage2(){
			s23 = NULL;
		}

		CoreOUT stage2(CoreOUT sCO12, Instruction* stage12, char currentChar){
			// Stage 2: get next PC and handle ACCEPT
			if (stage12->getType() == 1) s23 = stage12;
			else s23 = NULL;
			CoreOUT newPC;
			running = true;
			accept = false;
			valid = false;

			if (verbose) { printf("\t(PC%d)(S2) ", sCO12.getPC()); stage12->printType(sCO12.getPC());printf("\n");}

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
			if (verbose) { printf("\t(S3)"); stage23->printType(-1); printf("\n");}
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

		//TODO: this needs to become logic for interacting w/buffers. Should point to first
		//buffer in sliding window (CC_ID = 0)
		//Head?

		//TODO: W?/CC_ID both.
		// [CC_ID] [CC_ID + 1] ... [] CharID = currentChar + CC_ID
		vector<bool> CCIDBitmap; 
		unsigned short W;

		//settings
		bool verbose;

	public:
		Manager(Buffers* b, Core* c, bool dbg = false){
			core = c;
			buffers = b;
			verbose = dbg;
			//TODO: parametrize W. and remove it for size... useless.
			W = 1;
			CCIDBitmap.reserve(pow(2,W));
			for (int i = 0; i < pow(2,W); i++){
				CCIDBitmap.push_back(false);
			}


		}

		void updateBitmap(){
			//TODO: push or pull? is it the manager checking? OR core/buffers notifying? Seems notifying from ith active diagram. 
			//TODO: on push to buffer, pop from buffer, end of clock cycle?
			//Check buffers
			for (unsigned short i = 0; i < CCIDBitmap.size(); i++){
				CCIDBitmap[i] = !buffers->isEmpty(i) | ((core->getCO12().getCC_ID() == i)&core->getStage12() != NULL) | ((core->getCO23().getCC_ID() == i)&core->getStage23() != NULL)  ;
				//printf("Core12: %d\n", core->getCO12().getCC_ID() );
				//printf("Core23: %d\n", core->getCO23().getCC_ID() );
				//printf("CC_ID: %d conditions %d,%d,%d -> result: %d\n",i, !buffers->isEmpty(i), ((core->getCO12().getCC_ID() == i)&core->getStage12() != NULL) , ((core->getCO23().getCC_ID() == i)&core->getStage23() != NULL), (bool)CCIDBitmap[i]) ;
			}
			
		}


		bool runMultiChar(const char* input){
			
			// Signals handled by manager
			currentChar = 0; // In MultiChar it refers to first character in sliding window.


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

			// Load first instruction PC.
			buffers->pushTo(0, CoreOUT(0,0));
			CCIDBitmap[0] = true;

			if (verbose) printf("\nInitiating match of string %s\n", input);

			// Simulate clock cycle
			while (!core->isAccepted() && core->isRunning()){
			
				//TODO: fix print w/ additional info
				if (verbose) printf("[CC%d] Window first character: %c\n", CC, input[currentChar]);

				/* READ
				 * To simulate the stages being concurrent, all values used by stages 2 and 3 
				 * must be read at the start of the cycle, emulating values being read on rise. */

				// Check at the start whether each stage meets the conditions for being executed.
				//stage1Ready = !buffers->isEmpty(bufferHEAD); //TODO: new verification of stage1ready to account for multichar
				stage1Ready = buffers->hasInstructionReady(); //TODO: new verification of stage1ready to account for multichar
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
				if (stage1Ready) core->stage1(buffers->getPC(buffers->getFirstNotEmpty()));
				else {core->stage1();} //Empty run to clear signals.

				// Stage 2: sets valid, running and accept signals. 
				if (stage2Ready) {
					newPC = core->stage2(CO12, s12, input[currentChar+CO12.getCC_ID()]);

					// Handle the returned value, if it's a valid one. 
					if (core->isValid()){
						if (verbose) printf("\t\tPushing PC%d to FIFO%x\n", newPC.getPC(), newPC.getCC_ID()); 
						// Push to correct buffer
						buffers->pushTo(newPC.getCC_ID(), newPC); 
						buffers->printQueue(newPC.getCC_ID());
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

					if (verbose) printf("\t\tPushing PC%d to FIFO%x\n", newPC.getPC(),  newPC.getCC_ID()); 
					// Push to correct buffer
					buffers->pushTo(newPC.getCC_ID(), newPC); 
					buffers->printQueue(newPC.getCC_ID());
				}

				/* WRITEBACK 
				 * Values should only be modified at the end of the clock cycle */

				// Only if stage 1 was executed, consume the instruction that was loaded from the buffer.
				// Otherwise, it would risk consuming a value added by stage 2 in the same cycle.
				if (stage1Ready){
					if (verbose) printf("\t\tConsumed PC%d from FIFO%d, relating to character %c\n", buffers->getPC(buffers->getFirstNotEmpty()).getPC(), buffers->getFirstNotEmpty(), input[currentChar + core->getCO12().getCC_ID()]);
					buffers->popPC(buffers->getFirstNotEmpty());		
					if (verbose) printf("\t\tNext PC from FIFO%d: %d\n", buffers->getFirstNotEmpty(), buffers->getPC(buffers->getFirstNotEmpty()).getPC());
				} 
				//TODO: this becomes sliding window update (update also head/buffer select when sliding. Needs to account for stage 3 too, which it didn't in base case!!
				// If buffer was empty both now and at the start of the cycle, it's safe to deem it empty and switch to handling a new character TODO: check logic with multichar
				// TODO: cannot be else if anymore, can it?
				// TODO: updateBitmap() here
				//printf("Bitmap status bf: ");
				//for (int i=0; i< CCIDBitmap.size(); i++) printf("%d, ",(bool) CCIDBitmap[i]);
				//printf("\n");

				updateBitmap();
				printf("\tActive threads at end of cycle: ");
				for (int i=0; i< CCIDBitmap.size(); i++) {
					if ((bool)CCIDBitmap[i]) printf("%d ", i);
				}
				printf("\n");
				for (int i=0; i< CCIDBitmap.size(); i++) {
					buffers->printQueue(i);
				}

				if ((bool)CCIDBitmap[0] == false){ // Conditions for sliding the window.
										   
					
					//TODO: unsigned short i = slidingOP()	
					unsigned short slide;
					for (int i = 0; i < CCIDBitmap.size(); i++){
						if ((bool)CCIDBitmap[i]==true) {
							slide = i;
							break;
						}
					}
					currentChar += slide; // Move the window + i
					buffers->slide(slide);
					if (verbose) printf("\t\t%x Threads are inactive, sliding window. New first char in window: %c\n", slide, input[currentChar]);
					//TODO: I have my theoretical how-to-slide approach. Only need to keep buffers and CC_IDs coordinated. 
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
		bool multiChar = false; 
		bool verbose = true;
		bool hasProgram = false;

	public:
		SoftwareCICERO(unsigned short W = 1, bool dbg = false){

			if (W == 0) W = 1;

			if (W > 1) {

				multiChar = true;
				buffers = Buffers(pow(2,W));
				
			} else { 

				multiChar = false;
				buffers = Buffers(2);
			}

			hasProgram = false;
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

//TODO: split into headers and files to make it work like a decent library.

int main(void){
		
	SoftwareCICERO CICERO = SoftwareCICERO(1,true);
	CICERO.setProgram("./runningex");


	if(CICERO.match("abaababd")) printf("regex %d 	, input %d (len: %d)	, match True\n",0,0,256);
	else printf("regex %d 	, input %d (len: %d)	, match False\n",0,0,256);


	return 0;
}

