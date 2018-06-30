#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring> // std::sscanf
#include <vector>
#include <utility> // std::pair
#include <map> // bench ID (key) to integer number (value; count++)
#include <string>

using namespace std;


struct Gate{
	string fanout;
	vector<string> fanin;
	string type;
};

void readfile(fstream &fin, vector<string> &inputs, vector<string> &outputs, vector<Gate> &gates){

	string dump;

	while (getline(fin, dump, '\n')){
		// content into ss
		// dump is released and to be further reused
		// use dump only to check condition for if
		stringstream ss(dump); 

		// skip blank lines
		if ((dump[0] == '\0') || (dump[0] == '#')) continue; 

		// vector of INPUTs
		else if (dump[0] == 'I'){ 
			string temp;
			getline(ss, dump, '('); // dump is reused
			getline(ss, temp, ')');
			inputs.push_back(temp);
		}

		// vector of OUTPUTs	
		else if (dump[0] == 'O'){ 
			string temp;
			getline(ss, dump, '('); // dump is reused
			getline(ss, temp, ')');
			outputs.push_back(temp);
		}

		// vector of GATEs	
		else {
			Gate gate;
			// set fanout
			ss >> gate.fanout; 

			// set gatetype	
			getline(ss, dump, '(');

			if (dump == " = AND") gate.type = "g_AND";
			else if (dump == " = NAND") gate.type = "g_NAND";
			else if (dump == " = OR") gate.type = "g_OR";
			else if (dump == " = NOR") gate.type = "g_NOR";
			else if (dump == " = BUF") gate.type = "g_BUF";
			else if (dump == " = NOT") gate.type = "g_NOT";
			else if (dump == " = XOR") gate.type = "g_XOR";
			else if (dump == " = EQV") gate.type = "g_EQV";
			else break;

			// set fanin (may be multiple)
			getline(ss, dump, ')');
			stringstream subss(dump);

			while (getline(subss, dump, ',')){ // get rid of commas
				gate.fanin.push_back(dump);
				getline(subss, dump, ' '); // get rid of blanks
			}	
			gates.push_back(gate);								
		}	
		
		ss.clear(); // added to reuse
    	ss.str("");	// added to reuse
	}
}

vector<Gate> buildMiter(vector<string> &outputAs, vector<string> &outputBs){
	// initialization
	vector<Gate> gateMs;

	// set type: XORs 
	for (int i = 0; i < outputAs.size(); i++){
		Gate gatem;
		gatem.type = "g_XOR";
		// fix segmentation fault: wrong usage for vector
		gatem.fanin.push_back(outputAs[i]); // gatem.fanin[0] = outputAs[i]; is wrong
		gatem.fanin.push_back(outputBs[i]); // gatem.fanin[1] = outputBs[i]; is wrong
		gateMs.push_back(gatem);
	}

	// set type: final OR
	Gate gatef;
	gatef.type = "g_OR";
	for (int i = 0; i < gateMs.size(); i++){
		// fix segmentation fault: wrong usage for vector
		gatef.fanin.push_back(gateMs[i].fanout); // gatef.fanin[i] = gateMs[i].fanout; is wrong
	}
	gateMs.push_back(gatef);
	
	return gateMs;
}

int fanNums(vector<string> &inputAs, vector<string> &inputBs, 
			vector<string> &outputAs, vector<string> &outputBs,
			vector<Gate> &gateAs, vector<Gate> &gateBs, 
			map<string, int> &fanNum) { // add map<string, int> &fanNum

	// remove map<string, int> fanNum; 
	int count = 1; // set as key
	
	// all inputs in circuit A
	for (int i = 0; i < inputAs.size(); i++) {
		fanNum.insert(make_pair("A_"+inputAs[i], count++));
	}
	
	// all inputs in circuit B
	for (int i = 0; i < inputBs.size(); i++) {
		fanNum.insert(make_pair("B_"+inputBs[i], fanNum.find("A_" + inputBs[i])->second));
	}
	
	// all fanins in circuit A
	for (int i = 0; i < gateAs.size(); i++) { // all gates in circuit A
		for (int j = 0; j < gateAs[i].fanin.size(); j++){ // all fanins for a certain gate in A
			// key = fanID, value = count
			if(fanNum.find("A_" + gateAs[i].fanin[j])==fanNum.end()){
				fanNum.insert(make_pair("A_" + gateAs[i].fanin[j], count)); 
				count++;
			}
		}		
	}
	// cout << "fanins in circuit A: " << count << endl;

	// all fanouts in circuit A (without redundancy)
	for (int i = 0; i < gateAs.size(); i++) { // all gates in circuit A
		// only one fanout for one gate
		auto it1 = fanNum.find("A_" + gateAs[i].fanout); 
		if (it1 != fanNum.end());  // found in earlier entries
			// do nothing
		else { // not found, then add to map
			fanNum.insert(make_pair("A_" + gateAs[i].fanout, count));
			count++;
		}		
	}
	// cout << "add fanouts in circuit A: " << count << endl;

	// all fanins in circuit B (without redundancy)
	for (int i = 0; i < gateBs.size(); i++) { // all gates in circuit B
		for (int j = 0; j < gateBs[i].fanin.size(); j++) { // all fanins for a certain gate in B
			auto it2 = fanNum.find("B_" + gateBs[i].fanin[j]);
			if (it2 != fanNum.end());// found in earlier entries
				// do nothing
			else { // not found, then add to map
				fanNum.insert(make_pair("B_" + gateBs[i].fanin[j], count));
				count++;
			}
		}
	}
	// cout << "add fanins in circuit B: " << count << endl;

	// all fanouts in circuit B (without redundancy)
	for (int i = 0; i < gateBs.size(); i++) { // all gates in circuit B
		// only one fanout for one gate
		auto it3 = fanNum.find("B_" + gateBs[i].fanout); 
		if (it3 != fanNum.end());  // found in earlier entries
			// do nothing
		else { // not found, then add to map
			fanNum.insert(make_pair("B_" + gateBs[i].fanout, count));
			count++;
		}		
	}
	
	// cout << "add fanouts in circuit B: " << count << endl;

	// fanin and fanouts in miter structure
	vector<Gate> gateMs = buildMiter(outputAs, outputBs);
	
	// cout << "add fanins and fanouts in miter: " << count << endl;
	// cout << fanNum.size() << endl;
	
	return fanNum.size();
}

int lineNums(vector<Gate> &gateAs, vector<Gate> &gateBs, vector<Gate> &gateMs) {
	int lineNums = 1; // 1 for "p cnf X Y"

	for (int i = 0; i < gateAs.size(); i++){
		if (gateAs[i].type == "g_AND") lineNums += (gateAs[i].fanin.size() + 1);
		else if (gateAs[i].type == "g_NAND") lineNums += (gateAs[i].fanin.size() + 1);
		else if (gateAs[i].type == "g_OR") lineNums += (gateAs[i].fanin.size() + 1);
		else if (gateAs[i].type == "g_NOR") lineNums += (gateAs[i].fanin.size() + 1);
		else if (gateAs[i].type == "g_BUF") lineNums += 2;
		else if (gateAs[i].type == "g_NOT") lineNums += 2;
		else if (gateAs[i].type == "g_XOR") lineNums += 4;		
		else if (gateAs[i].type == "g_EQV") lineNums += 4;
	}

	for (int i = 0; i < gateBs.size(); i++){
		if (gateBs[i].type == "g_AND") lineNums += (gateBs[i].fanin.size() + 1);
		else if (gateBs[i].type == "g_NAND") lineNums += (gateBs[i].fanin.size() + 1);
		else if (gateBs[i].type == "g_OR") lineNums += (gateBs[i].fanin.size() + 1);
		else if (gateBs[i].type == "g_NOR") lineNums += (gateBs[i].fanin.size() + 1);
		else if (gateBs[i].type == "g_BUF") lineNums += 2;
		else if (gateBs[i].type == "g_NOT") lineNums += 2;
		else if (gateBs[i].type == "g_XOR") lineNums += 4;		
		else if (gateBs[i].type == "g_EQV") lineNums += 4;
	}

	for (int i = 0; i < gateMs.size(); i++){
		if (gateMs[i].type == "g_OR") lineNums += (gateMs[i].fanin.size() + 1);
		else if (gateMs[i].type == "g_XOR") lineNums += 4;		
	}

	return lineNums;
}

int main(int argc, char* argv[]){

	// ./ec [*_A.bench] [*_B.bench] [*.dimacs]

	//////////// read the input file /////////////

	fstream fin_A(argv[1]); //[*_A.bench]
	fstream fin_B(argv[2]); //[*_B.bench]

	vector<string> inputs_A, inputs_B, outputs_A, outputs_B;
	vector<Gate> gates_A, gates_B;	
	readfile(fin_A, inputs_A, outputs_A, gates_A);
	readfile(fin_B, inputs_B, outputs_B, gates_B);

	fin_A.close();
	fin_B.close();

	vector<Gate> gates_M = buildMiter(outputs_A, outputs_B);
	
	map<string, int> fanNum; // moved here

	// cout << "vector of input IDs in circuit A:" << endl;

	// for (int i = 0; i < inputs_A.size(); i++) {
	// 	cout << inputs_A[i] << endl;
	// }

	// cout << "vector of input IDs in circuit B:" << endl;

	// for (int i = 0; i < inputs_B.size(); i++) {
	// 	cout << inputs_B[i] << endl;
	// }

	// cout << "vector of output IDs in circuit A:" << endl;

	// for (int i = 0; i < outputs_A.size(); i++) {
	// 	cout << outputs_A[i] << endl;
	// }

	// cout << "vector of output IDs in circuit B:" << endl;

	// for (int i = 0; i < outputs_B.size(); i++) {
	// 	cout << outputs_B[i] << endl;
	// }

	// cout << "vector of Gates in circuit A:" << endl;
	

	// for (int i = 0; i < gates_A.size(); i++) {	
	// 	cout << "-- fanins :" << endl;
	// 	for (int j = 0; j < gates_A[i].fanin.size(); j++) {
	// 		cout << gates_A[i].fanin[j] << endl;
	// 	}	

	// 	cout << "-- fanout :" <<  endl;
	// 	cout << gates_A[i].fanout << endl;
	// 	cout << "-- type :" << endl;
	// 	cout << gates_A[i].type << endl;		

	// }

	// cout << "vector of Gates in circuit B:" << endl;
	
	// for (int i = 0; i < gates_B.size(); i++) {	
	// 	cout << "-- fanins :" << endl;
	// 	for (int j = 0; j < gates_B[i].fanin.size(); j++) {
	// 		cout << gates_B[i].fanin[j] << endl;
	// 	}	

	// 	cout << "-- fanout :" <<  endl;
	// 	cout << gates_B[i].fanout << endl;
	// 	cout << "-- type :" << endl;
	// 	cout << gates_B[i].type << endl;		

	// }

	// cout << "vector of Gates in Miter:" << endl;
	

	// for (int i = 0; i < gates_M.size(); i++) {	
	// 	cout << "-- fanins :" << endl;
	// 	for (int j = 0; j < gates_M[i].fanin.size(); j++) {
	// 		cout << gates_M[i].fanin[j] << endl;
	// 	}	

	// 	cout << "-- fanout :" <<  endl;
	// 	cout << gates_M[i].fanout << endl;
	// 	cout << "-- type :" << endl;
	// 	cout << gates_M[i].type << endl;		

	// }

	//////////// generate the output file /////////////
	fstream fout;
	fout.open(argv[3],ios::out); 

	fout << "p cnf " 
		 << fanNums(inputs_A, inputs_B, outputs_A, outputs_B, gates_A, gates_B, fanNum) - inputs_A.size() + outputs_A.size() + 1 << " " 
		 << lineNums(gates_A, gates_B, gates_M) << endl;

	// for gates in circuit A
	for (int i = 0; i < gates_A.size(); i++) {
		if (gates_A[i].type == "g_AND") { // multi-input
			for (int j = 0; j < gates_A[i].fanin.size(); j++) {
				fout << fanNum["A_"+gates_A[i].fanin[j]] << " -" << fanNum["A_" + gates_A[i].fanout] << " 0" << endl;
			}
			fout << fanNum["A_"+gates_A[i].fanout];
			for (int j = 0; j < gates_A[i].fanin.size(); j++) {
				fout << " -" << fanNum["A_"+gates_A[i].fanin[j]];
			}
			fout << " 0" << endl;
		}
		else if (gates_A[i].type == "g_NAND") { // multi-input
			for (int j = 0; j < gates_A[i].fanin.size(); j++) {
				fout << fanNum["A_" + gates_A[i].fanin[j]] << " " << fanNum["A_" + gates_A[i].fanout] << " 0" << endl;
			}
			fout << "-" << fanNum["A_" + gates_A[i].fanout];
			for (int j = 0; j < gates_A[i].fanin.size(); j++) {
				fout << " -" << fanNum["A_" + gates_A[i].fanin[j]];
			}
			fout << " 0" << endl;
		}
		else if (gates_A[i].type == "g_OR") { // multi-input
			for (int j = 0; j < gates_A[i].fanin.size(); j++) {
				fout << " -" << fanNum["A_" + gates_A[i].fanin[j]] << " " << fanNum["A_" + gates_A[i].fanout] << " 0" << endl;
			}
			fout << "-" << fanNum["A_"+gates_A[i].fanout];
			for (int j = 0; j < gates_A[i].fanin.size(); j++) {
				fout << " " << fanNum["A_" + gates_A[i].fanin[j]];
			}
			fout << " 0" << endl;
		}
		else if (gates_A[i].type == "g_NOR") { // multi-input
			for (int j = 0; j < gates_A[i].fanin.size(); j++) {
				fout << "-" << fanNum["A_" + gates_A[i].fanin[j]] << " -" << fanNum["A_" + gates_A[i].fanout] << " 0" << endl;
			}
			fout << fanNum["A_"+gates_A[i].fanout];
			for (int j = 0; j < gates_A[i].fanin.size(); j++) {
				fout << " " << fanNum["A_" + gates_A[i].fanin[j]];
			}
			fout << " 0" << endl;
		}
		else if (gates_A[i].type == "g_BUF") { // 1-input
			fout << "-" << fanNum["A_" + gates_A[i].fanin[0]] << " " << fanNum["A_" + gates_A[i].fanout] << " 0" << endl;
			fout << fanNum["A_" + gates_A[i].fanin[0]] << " -" << fanNum["A_" + gates_A[i].fanout] << " 0" << endl;
		}
		else if (gates_A[i].type == "g_NOT") { // 1-input
			fout << "-" << fanNum["A_" + gates_A[i].fanin[0]] << " -" << fanNum["A_" + gates_A[i].fanout] << " 0" << endl;
			fout << fanNum["A_" + gates_A[i].fanin[0]] << " " << fanNum["A_" + gates_A[i].fanout] << " 0" << endl;
		}
		else if (gates_A[i].type == "g_XOR") { // 2-input
			fout << "-" << fanNum["A_" + gates_A[i].fanin[0]] << " -" << fanNum["A_" + gates_A[i].fanin[1]]<< " -" << fanNum["A_"+gates_A[i].fanout] << " 0" << endl;
			fout << fanNum["A_" + gates_A[i].fanin[0]] << " " << fanNum["A_" + gates_A[i].fanin[1]]
				 << " -" << fanNum["A_" + gates_A[i].fanout] << " 0" << endl;
			fout << fanNum["A_" + gates_A[i].fanin[0]] << " -" << fanNum["A_" + gates_A[i].fanin[1]] 
				 << " " << fanNum["A_" + gates_A[i].fanout] << " 0" << endl;
			fout << "-" << fanNum["A_" + gates_A[i].fanin[0]] << " " << fanNum["A_" + gates_A[i].fanin[1]] << " "
				 << fanNum["A_"+gates_A[i].fanout] << " 0" << endl;
		}	
		else if (gates_A[i].type == "g_EQV") { // 2-input
			fout << fanNum["A_" + gates_A[i].fanin[0]] << " " << fanNum["A_" + gates_A[i].fanin[1]] 
				 << " " << fanNum["A_"+gates_A[i].fanout] << " 0" << endl;
			fout << fanNum["A_" + gates_A[i].fanin[0]] << " -" << fanNum["A_" + gates_A[i].fanin[1]] 
				 << " -" << fanNum["A_" + gates_A[i].fanout] << " 0" << endl;
			fout << "-" << fanNum["A_" + gates_A[i].fanin[0]] << " " << fanNum["A_" + gates_A[i].fanin[1]] 
				 << " -" << fanNum["A_" + gates_A[i].fanout] << " 0" << endl;
			fout << "-" << fanNum["A_" + gates_A[i].fanin[0]] << " -" << fanNum["A_" + gates_A[i].fanin[1]] 
				 << fanNum["A_" + gates_A[i].fanout] << " 0" << endl;
		}
	}

	// for gates in circuit B
	for (int i = 0; i < gates_B.size(); i++) {
		if (gates_B[i].type == "g_AND") { // multi-input
			for (int j = 0; j < gates_B[i].fanin.size(); j++) {
				fout << fanNum["B_" + gates_B[i].fanin[j]] << " -" << fanNum["B_" + gates_B[i].fanout] << " 0" << endl;
			}
			fout << fanNum["B_" + gates_B[i].fanout];
			for (int j = 0; j < gates_B[i].fanin.size(); j++) {
				fout << " -" << fanNum["B_" + gates_B[i].fanin[j]];
			}
			fout << " 0" << endl;
		}
		else if (gates_B[i].type == "g_NAND") { // multi-input
			for (int j = 0; j < gates_B[i].fanin.size(); j++) {
				fout << fanNum["B_" + gates_B[i].fanin[j]] << " " << fanNum["B_" + gates_B[i].fanout] << " 0" << endl;
			}
			fout << "-" << fanNum["B_"+gates_B[i].fanout];
			for (int j = 0; j < gates_B[i].fanin.size(); j++) {
				fout << " -" << fanNum["B_" + gates_B[i].fanin[j]];
			}
			fout << " 0" << endl;
		}
		else if (gates_B[i].type == "g_OR") { // multi-input
			for (int j = 0; j < gates_B[i].fanin.size(); j++) {
				fout << " -" << fanNum["B_" + gates_B[i].fanin[j]] << " " << fanNum["B_" + gates_B[i].fanout] << " 0" << endl;
			}
			fout << "-" << fanNum["B_" + gates_B[i].fanout];
			for (int j = 0; j < gates_B[i].fanin.size(); j++) {
				fout << " " << fanNum["B_" + gates_B[i].fanin[j]];
			}
			fout << " 0" << endl;
		}
		else if (gates_B[i].type == "g_NOR") { // multi-input
			for (int j = 0; j < gates_B[i].fanin.size(); j++) {
				fout << "-" << fanNum["B_" + gates_B[i].fanin[j]] << " -" << fanNum["B_" + gates_B[i].fanout] << " 0" << endl;
			}
			fout << fanNum["B_" + gates_B[i].fanout];
			for (int j = 0; j < gates_B[i].fanin.size(); j++) {
				fout << " " << fanNum["B_" + gates_B[i].fanin[j]];
			}
			fout << " 0" << endl;
		}
		else if (gates_B[i].type == "g_BUF") { // 1-input
			fout << "-" << fanNum["B_" + gates_B[i].fanin[0]] << " " << fanNum["B_" + gates_B[i].fanout] << " 0" << endl;
			fout << fanNum["B_" + gates_B[i].fanin[0]] << " -" << fanNum["B_" + gates_B[i].fanout] << " 0" << endl;
		}
		else if (gates_B[i].type == "g_NOT") { // 1-input
			fout << "-" << fanNum["B_" + gates_B[i].fanin[0]] << " -" << fanNum["B_" + gates_B[i].fanout] << " 0" << endl;
			fout << fanNum["B_" + gates_B[i].fanin[0]] << " " << fanNum["B_" + gates_B[i].fanout] << " 0" << endl;
		}
		else if (gates_B[i].type == "g_XOR") { // 2-input
			fout << "-" << fanNum["B_" + gates_B[i].fanin[0]] << " -" << fanNum["B_" + gates_B[i].fanin[1]] 
				 << " -" << fanNum["B_" + gates_B[i].fanout] << " 0" << endl;
			fout << fanNum["B_" + gates_B[i].fanin[0]] << " " << fanNum["B_" + gates_B[i].fanin[1]] 
				 << " -" << fanNum["B_" + gates_B[i].fanout] << " 0" << endl;
			fout << fanNum["B_" + gates_B[i].fanin[0]] << " -" << fanNum["B_" + gates_B[i].fanin[1]] 
				 << " " << fanNum["B_" + gates_B[i].fanout] << " 0" << endl;
			fout << "-" << fanNum["B_" + gates_B[i].fanin[0]] << " " << fanNum["B_" + gates_B[i].fanin[1]] << " "
				 << fanNum["B_" + gates_B[i].fanout] << " 0" << endl;
		}	
		else if (gates_B[i].type == "g_EQV") { // 2-input
			fout << fanNum["B_" + gates_B[i].fanin[0]] << " " << fanNum["B_" + gates_B[i].fanin[1]] 
				 << " " << fanNum["B_"+gates_B[i].fanout] << " 0" << endl;
			fout << fanNum["B_" + gates_B[i].fanin[0]] << " -" << fanNum["B_" + gates_B[i].fanin[1]] 
				 << " -" << fanNum["B_" + gates_B[i].fanout] << " 0" << endl;
			fout << "-" << fanNum["B_" + gates_B[i].fanin[0]] << " " << fanNum["B_" + gates_B[i].fanin[1]] 
				 << " -" << fanNum["B_" + gates_B[i].fanout] << " 0" << endl;
			fout << "-" << fanNum["B_" + gates_B[i].fanin[0]] << " -" << fanNum["B_" + gates_B[i].fanin[1]] <<" "
				 << fanNum["B_" + gates_B[i].fanout] << " 0" << endl;
		}
	}

	// for gates in miter structure
	
	int miter_output_count = 1-inputs_A.size();
	
	for (int i = 0; i < gates_M.size(); i++) {
		if (gates_M[i].type == "g_XOR") { // 2-input
			fout << "-" << fanNum["A_" + gates_M[i].fanin[0]] << " -" << fanNum["B_" + gates_M[i].fanin[1]] << " -" << fanNum.size()+miter_output_count << " 0" << endl;
			fout << fanNum["A_" + gates_M[i].fanin[0]] << " " << fanNum["B_" + gates_M[i].fanin[1]] 
				 << " -" << fanNum.size() + miter_output_count << " 0" << endl;
			fout << fanNum["A_" + gates_M[i].fanin[0]] << " -" << fanNum["B_" + gates_M[i].fanin[1]] 
				 << " " << fanNum.size() + miter_output_count << " 0" << endl;
			fout << "-" << fanNum["A_" + gates_M[i].fanin[0]] << " " << fanNum["B_" + gates_M[i].fanin[1]] << " " << fanNum.size()+miter_output_count << " 0" << endl;
			
			++miter_output_count;
		}	
	}
	
	int wireNum = fanNum.size() - inputs_A.size();
	
	for (int i = wireNum + 1; i <= wireNum + outputs_A.size(); i++) {
		fout << " -" << i << " " << wireNum + outputs_A.size() + 1 << " 0" << endl;
	}
	fout << "-" << wireNum + outputs_A.size() + 1;
	for (int i = wireNum + 1; i <= wireNum + outputs_A.size(); i++) {
		fout << " " << i;
	}
	fout << " 0" << endl;
	fout << wireNum + outputs_A.size() + 1 << " 0" << endl;
	
	fout.close();

	return 0;
}