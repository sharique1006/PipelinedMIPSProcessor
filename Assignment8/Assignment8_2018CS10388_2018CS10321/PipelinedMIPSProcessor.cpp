#include <bits/stdc++.h>
#include <iostream>
#include <vector>
#include <string.h>
#include <fstream>

using namespace std;


//Created an Instruction struct with parameters - a string instruction, a string function (lw or sw or add etc.), 
//destination and source registers rd, rs & rt, shift amount shamt, offset and a tuple ALUResult which is the result of ALU after execution
struct INSTRUCTION {
	string instruction;
	string function;
	int rd;
	int rs;
	int rt;
	int shamt;
	int offset;
	tuple <string, int, int, float> ALUResult;
};

//initialized PC, return address and clock cycles
int PC = -1;
int ra = 0;
int clock_cycles = 0;

//initialized instruction memory, register file, memory array and a pipeline array of 5 stages
string instruction_memory[1000];
float register_file[32];
float memory_array[4096];
struct INSTRUCTION pipeline_array[5];

//a power function to calculate shift amount
int powerof2 (int x) {
	int result = 1;
	for(int i = 1; i <= x; i++) {
		result = result*2;
	}
	return result;
}

//Stage1 - Instruction Fetch, returns instruction from the instruction memory for given PC
string IF() {
	return instruction_memory[PC];
}

//Stage2 - Instruction Decode, tokenizes the instruction to get the Read, Write Registers
INSTRUCTION ID(struct INSTRUCTION instruct) {
	string instr = instruct.instruction;
	char *tokenized_inst[10];
	int len = instr.length();
	char str[len+1];
	strcpy(str, instr.c_str());
	char *token = strtok(str, " ");
	int cnt = 0;

	//tokenized the instruction
	while(token != NULL) {
		if(strchr(token, ',')) {
			token[strlen(token)-1] = '\0';
		}
		if(strcmp(token, "$ra") != 0 && strchr(token, '$')) {
			for(int i = 1; i < strlen(token); i++) {
				token[i-1] = token[i];
			}
			token[strlen(token)-1] = '\0';
		}
		tokenized_inst[cnt] = token;
		cnt++;
		token = strtok(NULL, " ");
	}

	//when lw or sw - assigned values to rt and offset
	if(strcmp(tokenized_inst[0], "lw") == 0 || strcmp(tokenized_inst[0], "sw") == 0) {
		instruct.function = tokenized_inst[0];
		instruct.rd = 0;
		instruct.rs = 0;
		instruct.rt = atoi(tokenized_inst[1]);
		instruct.shamt = 0;
		instruct.offset = atoi(tokenized_inst[2]);
	}
	//when add, sub - assigned values to rd, rs, rt and rd
	else if(strcmp(tokenized_inst[0], "add") == 0 || strcmp(tokenized_inst[0], "sub") == 0) {
		instruct.function = tokenized_inst[0];
		instruct.rd = atoi(tokenized_inst[1]);
		instruct.rs = atoi(tokenized_inst[2]);
		instruct.rt = atoi(tokenized_inst[3]);
		instruct.shamt = 0;
		instruct.offset = 0;
	}
	//when sll or srl - assigned values to rd, rt and shamt
	else if(strcmp(tokenized_inst[0], "sll") == 0 || strcmp(tokenized_inst[0], "srl") == 0) {
		instruct.function = tokenized_inst[0];
		instruct.rd = atoi(tokenized_inst[1]);
		instruct.rs = 0;
		instruct.rt = atoi(tokenized_inst[2]);
		instruct.shamt = atoi(tokenized_inst[3]);
		instruct.offset = 0;
	}
	//when beq, bne - assigned values to rs, rt and offset
	else if(strcmp(tokenized_inst[0], "beq") == 0 || strcmp(tokenized_inst[0], "bne") == 0) {
		instruct.function = tokenized_inst[0];
		instruct.rd = 0;
		instruct.rs = atoi(tokenized_inst[1]);
		instruct.rt = atoi(tokenized_inst[2]);
		instruct.shamt = 0;
		instruct.offset = atoi(tokenized_inst[3]);
	}
	//when blez or bgtz - assigned values to rs and offset
	else if(strcmp(tokenized_inst[0], "blez") == 0 || strcmp(tokenized_inst[0], "bgtz") == 0) {
		instruct.function = tokenized_inst[0];
		instruct.rd = 0;
		instruct.rs = atoi(tokenized_inst[1]);
		instruct.rt = 0;
		instruct.shamt = 0;
		instruct.offset = atoi(tokenized_inst[2]);
	}
	//when jal or j assigned values to offset
	else if(strcmp(tokenized_inst[0], "jal") == 0 || strcmp(tokenized_inst[0], "j") == 0) {
		instruct.function = tokenized_inst[0];
		instruct.rd = 0;
		instruct.rs = 0;
		instruct.rt = 0;
		instruct.shamt = 0;
		instruct.offset = atoi(tokenized_inst[1]);
	}
	//when jr rs is 31, register for return address
	else if(strcmp(tokenized_inst[0], "jr") == 0) {
		instruct.function = tokenized_inst[0];
		instruct.rd = 0;
		instruct.rs = 31;
		instruct.rt = 0;
		instruct.shamt = 0;
		instruct.offset = 0;
	}
	//to exit the code
	else if(strcmp(tokenized_inst[0], "exit") == 0) {
		instruct.function = tokenized_inst[0];
		instruct.rd = 0;
		instruct.rs = 0;
		instruct.rt = 0;
		instruct.shamt = 0;
		instruct.offset = 0;
	}
	return instruct;
}

//Stage3 - Execute Step, returns a tuple storing the write address, the value to be written in writeback and whether to read or write from the memory
tuple<string, int, int, float> EX(struct INSTRUCTION instruct) {
	//return the register to be written from and the memory location
	if(instruct.function == "lw") {
		return make_tuple("lw", instruct.rt, instruct.offset, 0.0);
	}
	//return the register to be read and memory location where to store 
	else if(instruct.function == "sw") {
		return make_tuple("sw", instruct.rt, instruct.offset, 0.0);
	}
	//return the result after adding the values in the register
	else if(instruct.function == "add") {
		float result = register_file[instruct.rs] + register_file[instruct.rt];
		return make_tuple("add", instruct.rd, 0, result);
	}
	//return the result after subtracting the values in the register
	else if (instruct.function == "sub") {
		float result = register_file[instruct.rs] - register_file[instruct.rt];
		return make_tuple("sub", instruct.rd, 0, result);
	}
	//return the result after shifting left
	else if(instruct.function == "sll") {
		float result = register_file[instruct.rt]*powerof2(instruct.shamt);
		return make_tuple("sll", instruct.rd, 0, result);
	}
	//return th result after shifting right
	else if(instruct.function == "srl") {
		int res = register_file[instruct.rt]/powerof2(instruct.shamt);
		float result = (float)res;
		return make_tuple("srl", instruct.rd, 0, result);
	}
	else if(instruct.function == "beq") {
		//if the values in the register are equal set PC to offset
		if(register_file[instruct.rs] == register_file[instruct.rt]) {
			PC = instruct.offset;
			return make_tuple("beq", 0, instruct.offset, 0.0);
		}
	}
	else if(instruct.function == "bne") {
		//if the values in the register are not eqaul set PC to offset
		if(register_file[instruct.rs] != register_file[instruct.rt]) {
			PC = instruct.offset;
			return make_tuple("bne", 0, instruct.offset, 0.0);
		}
	}
	else if(instruct.function == "blez") {
		//if the value in register is less than equal to 0 set PC to offset
		if(register_file[instruct.rs] <= 0) {
			PC = instruct.offset;
			return make_tuple("blez", 0, instruct.offset, 0.0);
		}
	}
	else if(instruct.function == "bgtz") {
		//if the value in the register is greater than 0 set PC to offset
		if(register_file[instruct.rs] > 0) {
			PC = instruct.offset;
			return make_tuple("bgtz", 0, instruct.offset, 0.0);
		}
	}
	return make_tuple("none", 0, 0, 0.0);
}

//Stage4 - Read from memory or Write into the memory
void MEM(tuple <string,int,int,float> ALURes) {
	//Read from the memory
	if(get<0>(ALURes) == "lw") {
		register_file[get<1>(ALURes)] = memory_array[get<2>(ALURes)];
	}
	//Write into the memory
	else if(get<0>(ALURes) == "sw") {
		memory_array[get<2>(ALURes)] = register_file[get<1>(ALURes)];
	}
}

//Stage5 - WriteBack the ALU Result into the destination register
void WB(tuple <string,int,int,float> ALURes) {
	if(get<0>(ALURes) == "add" || get<0>(ALURes) == "sub" || get<0>(ALURes) == "sll" || get<0>(ALURes) == "srl") {
		register_file[get<1>(ALURes)] = get<3>(ALURes);
	}
}

//Function to display the pipeline array, to show the instruction present in each stage
void display() {
	if(pipeline_array[0].instruction != "begin") {
		cout <<"IF : "<<pipeline_array[0].instruction<<endl;
	}
	if(pipeline_array[1].instruction != "begin") {
		cout <<"ID : "<<pipeline_array[1].instruction<<endl;
	}
	if(pipeline_array[2].instruction != "begin") {
		cout <<"EX : "<<pipeline_array[2].instruction<<endl;
	}
	if(pipeline_array[3].instruction != "begin") {
		cout <<"MEM : "<<pipeline_array[3].instruction<<endl;
	}
	if(pipeline_array[4].instruction != "begin") {
		cout <<"WB : "<<pipeline_array[4].instruction<<endl;
	}
}

//Function to check if all the instructions have been completed
int Not_Completed() {
	if(pipeline_array[0].instruction == "begin" && pipeline_array[1].instruction == "begin" && pipeline_array[2].instruction == "begin" && pipeline_array[3].instruction == "begin" && pipeline_array[4].instruction == "begin") {
		return 1;
	}
	return 0;
}

//Function to Stall the pipeline till the branch instruction gets executed so that we know if PC is to be set to offset or to next instruction
void BRANCH_STALLING(int storePC, struct INSTRUCTION initialInstr) {
	cout <<"BRANCH STALLING"<<endl;
	//Store the value of PC if branch instruction fails
	ra = storePC+1;
	pipeline_array[0] = initialInstr;
	int k = 0;
	//Stalling the pipeline by 2 cycles so that we know the result of the branch and get correct PC value
	while (k != 2) {
		clock_cycles++;
		cout <<"CLOCK CYCLE : "<<clock_cycles<<endl;
		pipeline_array[1] = ID(pipeline_array[1]);
		pipeline_array[2].ALUResult = EX(pipeline_array[2]);
		MEM(pipeline_array[3].ALUResult);
		WB(pipeline_array[4].ALUResult);
		display();
		pipeline_array[4] = pipeline_array[3];
		pipeline_array[3] = pipeline_array[2];
		pipeline_array[2] = pipeline_array[1];
		pipeline_array[1] = pipeline_array[0];
		pipeline_array[0] = initialInstr;
		k++;
	}
	//if branch fails set PC to previous value + 1
	if(get<0>(pipeline_array[3].ALUResult) == "none") {
			PC = storePC+1;
	}
}

//Function to Stall the pipeline in case of a hazard
void STALL(struct INSTRUCTION initialInstr) {
	cout <<"DATA HAZARD STALLING: "<<endl;
	pipeline_array[0] = initialInstr;
	int k = 0;
	int sPC = PC;
	//Stall by 2 clock cycles so that Write Back is completed in the resgister which is to be read from.
	while(k != 2) {
		clock_cycles++;
		cout <<"CLOCK CYCLE : "<<clock_cycles<<endl;
		pipeline_array[1] = ID(pipeline_array[1]);
		pipeline_array[2].ALUResult = EX(pipeline_array[2]);
		MEM(pipeline_array[3].ALUResult);
		WB(pipeline_array[4].ALUResult);
		display();
		pipeline_array[4] = pipeline_array[3];
		pipeline_array[3] = pipeline_array[2];
		pipeline_array[2] = pipeline_array[1];
		pipeline_array[1] = initialInstr;
		pipeline_array[0] = initialInstr;
		k++;
	} 
	PC = sPC;
}

//Harard Detection Unit
void DATA_HAZARD_STALLING(struct INSTRUCTION initialInstr) {
	pipeline_array[1] = ID(pipeline_array[1]);
	struct INSTRUCTION temp;
	temp.instruction = instruction_memory[PC];
	temp = ID(temp);
	//Hazard Detection for cases like - reading immediately writing back in the register
	if(pipeline_array[1].function != "lw" && pipeline_array[1].function != "sw" && pipeline_array[1].function != "begin") {
		//For R-format instruction check if prev rd is equal to current rs or current rt
		if(pipeline_array[1].rd == temp.rs || pipeline_array[1].rd == temp.rt) {
			STALL(initialInstr);
		}
	}
	else {
		//For lw instruction check if prev rt = current rs
		if((pipeline_array[1].function != temp.function) && (pipeline_array[1].rt == temp.rs || pipeline_array[1].rt == temp.rt)) {
			STALL(initialInstr);
		}
	}
}

//Main Function
int main(int argc, char* argv[]) {
	ifstream infile;
	//Open the instruction file
	infile.open(argv[1]);
	string str;
	int index = 0;
	//Read the instructions from the file and store in the instruction memory
	while(getline(infile, str)) {
		instruction_memory[index] = str;
		index++;
	}
	int num_of_instructions = index - 1;
	//initialize in the register file
	for (int i = 0; i < 32; i++) {
		register_file[i] = 0;
	}
	//initialize the memory array
	memory_array[1024] = 3;
	memory_array[1025] = 5;
	memory_array[1026] = 2;
	memory_array[1027] = 6;
	memory_array[1028] = 2;
	memory_array[1029] = 3;
	memory_array[1030] = 10;
	//initialize the pipeline array
	for (int i = 0; i < 5; i++) {
		pipeline_array[i].instruction = "begin";
		pipeline_array[i].function = "begin";
		pipeline_array[i].rs = -1;
		pipeline_array[i].rd = -1;
		pipeline_array[i].rt = -1;
		pipeline_array[i].offset = -1;
		pipeline_array[i].ALUResult = make_tuple("initial", 0, 0, 0.0);
	}
	struct INSTRUCTION initialInstr = pipeline_array[0];
	//Begin the Simulation
	cout <<"Simulation Started :"<<endl;
	//Continue the pipeline while not exit
	while(pipeline_array[0].instruction != "exit") {
		//Move the instruction one step ahead that is instruction in stage 1 goes to stage 2, instruction in stage 2 goes to stage 3 and so on
		pipeline_array[4] = pipeline_array[3];
		pipeline_array[3] = pipeline_array[2];
		pipeline_array[2] = pipeline_array[1];
		pipeline_array[1] = pipeline_array[0];
		struct INSTRUCTION idex = ID(pipeline_array[1]);
		int storePC = PC;
		//If the instruction is jr set PC to return address
		if(idex.function == "jr") {
			PC = ra;
		}
		//if it is a jump instruction set PC to offset
		else if(idex.function == "jal" || idex.function == "j") {
			ra = PC+1;
			PC = idex.offset;
		}
		//Stall the pipeline in case of branch instructions
		else if(idex.function == "beq" || idex.function == "bne" || idex.function == "blez" || idex.function == "bgtz") {
			BRANCH_STALLING(storePC, initialInstr);
		}
		//Check for Hazard if found the Hazard Detection will stall the pipeline
		else {
			PC++;
			DATA_HAZARD_STALLING(initialInstr);
		}
		//Fetch the instruction in pipeline_array[0]
		pipeline_array[0].instruction = IF();
		//Decode the instruction in pipeline_array[1]
		pipeline_array[1] = ID(pipeline_array[1]);
		//Execute the instruction in pipeline_array[2]
		pipeline_array[2].ALUResult = EX(pipeline_array[2]);
		//Stage4 - Read Write in the Memory
		MEM(pipeline_array[3].ALUResult);
		//Stage5 - WriteBack Step
		WB(pipeline_array[4].ALUResult);
		//Increase the number of clock cycles
		clock_cycles++;
		cout <<"CLOCK CYCLE : "<<clock_cycles<<endl;
		cout <<"PC = "<<PC<<endl;
		//Display the pipeline
		display();
	}
	//Since all instructions are fetched complete the simulation
	cout <<"Completing the Simulation :"<<endl;
	//Simulate while not completed
	while(Not_Completed() == 0) {
		//Move the instructions in pipeline array
		pipeline_array[4] = pipeline_array[3];
		pipeline_array[3] = pipeline_array[2];
		pipeline_array[2] = pipeline_array[1];
		pipeline_array[1] = pipeline_array[0];
		pipeline_array[0] = initialInstr;
		//Decode the instruction in pipeline_array[1]
		pipeline_array[1] = ID(pipeline_array[1]);
		//Execute the instruction in pipeline_array[2]
		pipeline_array[2].ALUResult = EX(pipeline_array[2]);
		//Stage4 - Read Write in the Memory
		MEM(pipeline_array[3].ALUResult);
		//Stage5 - WriteBack Step
		WB(pipeline_array[4].ALUResult);
		//Increase the number of clock cycles
		clock_cycles++;
		cout <<"CLOCK CYCLE : "<<clock_cycles<<endl;
		display();
	}
	//Since exit instruction goes for 2 clock cycles it's not to be counted
	int total_number_of_clock_cycles = clock_cycles - 2;
	float ipc = (1.0*num_of_instructions)/total_number_of_clock_cycles;
	//Display the Register File
	for (int i = 0; i < 32; i++) {
			cout <<"Reg "<<i<<" : "<<register_file[i]<<endl;
	}
	//Display the Memory Array
	for (int i = 1024; i < 1050; i++) {
		if(memory_array[i] != 0) {
			cout <<"MEM "<<i<<" : "<<memory_array[i]<<endl;
		}
	}
	//Display the Total number of clock cycles
	cout <<"TOTAL NUMBER OF CLOCK CYCLES = "<<total_number_of_clock_cycles<<endl;
	cout <<"IPC = "<<ipc<<endl;
}