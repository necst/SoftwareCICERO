#include <iostream>
#include <queue>
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

class CoreOUT {
	private:
		unsigned short PC;
		bool active;
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
		
		
		void printType(){
			switch (this->getType())
			{
			case 0:
				printf("ACCEPT");
				break;
			case 1:
				printf("SPLIT");
				break;
			case 2:
				printf("MATCH");
				break;
			case 3:
				printf("JMP");
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
				printf("NOT_MATCH");
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
		// abstract print? nah just copy objdump.
};

class Buffers {
	private:
		vector<queue<unsigned short>> buffers;
		int size;
	public:
		Buffers(int n){ 
			size = n;
			buffers.reserve(n);
			for (int i = 0; i < n; i++){
				buffers.push_back(queue<unsigned short>());
			}
		}

		queue<unsigned short> getFIFO(int n){return buffers[n];}
		//int getCurrent(){return buffers[activeBuffer];};
		bool isEmpty(unsigned short target){
			return buffers[target].empty();
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
				fprintf(stderr, "[X] Pushing to non-existing buffer.");
		}
};



class Core {
	private:
		Instruction* program;
		Buffers* buffers;
		
		// String accepted
		bool accept;
		// Seen from HDL 
		bool valid;
		bool running;

		//Inter-phase registers
		Instruction* s12;
		Instruction* s23;
		unsigned short PC12;

		//settings
		bool verbose = true;

	public:
		Core(Instruction* p, Buffers* b){
			buffers = b;
			program = p;
			accept = false;
			valid = false;
			running = true;
			s12 = NULL;
			s23 = NULL;
			PC12 = -1;
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
			s12 = NULL;
		}
		void stage1(unsigned short PC){
			// WRONG; NEEDS TO GET PC FROM BUFFERS.
			s12 = &program[PC];
			PC12 = PC;
			if (verbose) { printf("\t(PC%x)(S1) ",PC); s12->printType(); printf("\n");}
		};

		CoreOUT stage2(unsigned short sPC12, Instruction* stage12, char currentChar){
			// Stage 2: get next PC and handle ACCEPT
			s23 = stage12;
			CoreOUT newPC;
			running = true;
			accept = false;
			valid = false;

			if (verbose) { printf("\t(PC%x)(S2) ", sPC12); stage12->printType();printf("\n");}

			switch(stage12->getType()){
				//TODO: check values and add end without matching.
				case ACCEPT:

					//TODO: if end of string
					running = false;
					accept = true;
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
						newPC = CoreOUT() ;
					}

					break;

				case JMP:
					valid = true;
					newPC = (CoreOUT(stage12->getData(), true));
					break;
				case MATCH_ANY:
					valid = true;
					newPC = (CoreOUT(sPC12+1, false));
					break;
				case ACCEPT_PARTIAL:

					accept = true;
					newPC = CoreOUT();

					break;
				case NOT_MATCH:

					if (char(stage12->getData())!=currentChar){
						valid = true;
						newPC = CoreOUT( sPC12+1, false);
					} else {
						newPC = CoreOUT();
					}
					break;

				default:
					//TODO some kind of error msg
					newPC =  CoreOUT();
					break;
					}

			return newPC;


		};
		CoreOUT stage3(Instruction* stage23){
			if (verbose) { printf("\t(S3)"); stage23->printType(); printf("\n");}
			CoreOUT newPC = CoreOUT(stage23->getData(), true);
			s23 = NULL;
			return newPC;
		};
};

class Manager {
	private:
		Buffers* buffers;
		Core* core;
		// Manager signals
		unsigned short currentChar;

		//settings
		bool verbose = true;;

	public:
		Manager(Buffers* b, Core* c){
			core = c;
			buffers = b;
		}

		// init(){};
		//
		// TODO: this is base match.
		bool runBase(const char* input){
			
			int CC = 0;
			CoreOUT newPC;
			currentChar = 0;
			bool bufferSelection = 0;
			buffers->pushTo(bufferSelection, 0);

			bool stage2Ready = false;
			bool stage1Ready = false;
			bool stage3Ready = false;
			Instruction* s12;
			Instruction* s23;
			unsigned short PC12;

			// Simulate clock cycle
			if (verbose) printf("\nInitiating match of string %s\n", input);

			while (!core->isAccepted() && input[currentChar]!= '\0'){
			

				if (verbose) printf("[CC%x] Char:%c, Active buffer: FIFO%x\n", CC, input[currentChar], bufferSelection);

				// Read on rise
				// Only propagate forward

				// To simulate the stages being concurrent, all values used by stages 2 and 3 must be read at the start of the cycle, emulating values being read on rise. 
				stage1Ready = !buffers->isEmpty(bufferSelection);
				stage2Ready = core->isStage2Ready();
				stage3Ready = core->isStage3Ready();
				s12 = core->getStage12();
				s23 = core->getStage23();
				PC12 = core->getPC12();


				// Stage 1: retrieve newPC from active buffer and load instruction.
				if (stage1Ready) core->stage1(buffers->getPC(bufferSelection));
				else {core->stage1();}

				if (stage2Ready) {
					newPC = core->stage2(PC12, s12, input[currentChar]);
					// PUSH AND WRITES MUST BE DONE AT END OF CLOCK.
					if (core->isValid()){
						// Push to correct buffer
						if (newPC.isActive()){
							if (verbose) printf("\t\tPushing PC%x to active FIFO%x\n", newPC.getPC(), bufferSelection);
							buffers->pushTo(bufferSelection, newPC.getPC());
						} else {
							if (verbose) printf("\t\tPushing PC%x to inactive FIFO%x\n", newPC.getPC(), !bufferSelection);
							buffers->pushTo(!bufferSelection, newPC.getPC());
						}
						// update currentchar
					} else if (core->isAccepted()){
						// return success.
						return true;
					} else if (!core->isRunning()){
						// shit ended.
					} 

				} 


				if (stage3Ready){

					newPC = core->stage3(s23);

					if (core->isValid()){
						// Push to correct buffer
						if (newPC.isActive()){
							if (verbose) printf("\t\tPushing PC%x to active FIFO%x\n", newPC.getPC(), bufferSelection);
							buffers->pushTo(bufferSelection, newPC.getPC());
						} else {
							if (verbose) printf("\t\tPushing PC%x to inactive FIFO%x\n", newPC.getPC(), !bufferSelection);
							buffers->pushTo(!bufferSelection, newPC.getPC());
						}
					}
				}



				if (stage1Ready){
					
					if (verbose) printf("\t\tConsumed PC%x from active FIFO%x. Continue operating on char %c\n", buffers->getPC(bufferSelection), bufferSelection, input[currentChar]);
					buffers->popPC(bufferSelection);				
				} 

				else if (buffers->isEmpty(bufferSelection)){ // Buffer ping pong.
					currentChar ++;
					if (verbose) printf("\t\tFIFO%x is empty, activating FIFO%x and moving to char %c\n", bufferSelection, !bufferSelection, input[currentChar]);
					bufferSelection = !bufferSelection;
				}
				CC++;
			

			}

			return false;

			// core.reset;
			// buffers.flush;

		}
};

// Wrapper class that holds and inits all components.
class SoftwareCICERO {
	private:

		// Components
		Instruction program[INSTR_MEM_SIZE];
		Buffers buffers = Buffers(2);
		Core core = Core(&program[0], &buffers);
		Manager manager = Manager(&buffers, &core);

		// Settings
		bool multiChar = false; 
		bool verbose = true;
		bool hasProgram = false;

	public:
		SoftwareCICERO(){

			multiChar = false;
			hasProgram = false;
			buffers = Buffers(2);
			core = Core(&program[0], &buffers);
			manager = Manager(&buffers, &core);
			verbose = true;
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
					fprintf(stderr, "[X] Program memory exceeded. Only the first %x instructions were read.", INSTR_MEM_SIZE);
				}

				hasProgram = true;

			} else {
				fprintf(stderr, "[X] Could not open program file for reading.");
			}
		};
		
		bool match(const char* input ){

			//TODO: checks on input string not being malformed.

			if (!hasProgram){
				fprintf(stderr, "[X] No program is loaded to match the string against.");
				return NULL;
			} 

			if (!multiChar){
				return manager.runBase(input);
			} else return false;
		}
};

int main(void){
		
	SoftwareCICERO CICERO = SoftwareCICERO();
	CICERO.setProgram("a.out");
	bool match = CICERO.match("ababcd");
	
	if (match) printf("Success!");

	return 0;
}
