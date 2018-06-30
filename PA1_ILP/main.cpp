#include <cstring>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>

using namespace std;

/*

two maps

vias_org_type 
	key: original vias coor
	value: vector of valid neighbor types (integer) 

vias_adj_coor 
	key: legitimate neighbor coor
	value: vector of pair of original vias coor and type (integer)

*/


struct Coordinates{
	int x;
	int y;
};

bool cocomp(Coordinates lhs, Coordinates rhs) {
	if (lhs.x < rhs.x) 
		return true;
	if (lhs.x == rhs.x && lhs.y < rhs.y) 
		return true;
	return false;
}

int main(int argc, char* argv[]){

	//////////// read the input file /////////////

	fstream fin(argv[1]);
	//cout << "argv[1] = " << argv[1] << endl; 
	int vias_count;
	fin >> vias_count;
	//cout << "vias_count = " << vias_count << endl; 
	
	bool(*co_pt)(Coordinates, Coordinates) = cocomp;
    map<Coordinates, vector<int>, bool(*)(Coordinates, Coordinates)> vias_org_type(co_pt); 
    //function pointer

	for (int i = 0; i < vias_count; ++i) {
		vector<int> neighbor; // empty vector
		Coordinates coordinate;
		fin >> coordinate.x;
		fin >> coordinate.y;
		vias_org_type.insert(make_pair(coordinate, neighbor));
	}
	//cout << "vias_org_type.size() = " << vias_org_type.size() << endl;

	/*
	cout << "vias_org_type contains: " ;
	for (auto it = vias_org_type.begin(); it != vias_org_type.end(); it++) {
		cout << "(" << it->x << ", " << it->y << ") ,";
	}
	cout << endl;
	*/

	fin.close();

	//////////// calculate the constraints /////////////
	
	Coordinates vias_u, vias_r, vias_d, vias_l;
	map<Coordinates, vector<pair<Coordinates, int>>, bool(*)(Coordinates, Coordinates)> vias_adj_coor(co_pt); 
	//function pointer
	
	for(auto it = vias_org_type.begin(); it != vias_org_type.end(); it++) {		
		vias_u.x = it->first.x;
		vias_u.y = it->first.y + 1;
		vias_r.x = it->first.x + 1;
		vias_r.y = it->first.y;
		vias_d.x = it->first.x;
		vias_d.y = it->first.y - 1;
		vias_l.x = it->first.x - 1;
		vias_l.y = it->first.y;

		auto it_u = vias_org_type.find(vias_u);
		
		//upper neighbor of current it not within original vias 
		if(it_u == vias_org_type.end()) { //valid neighbor

			//1. renew value of the first map
			it->second.push_back(1); // can insert neighbor type (upper)

			//2. insert to second map
			//key: valid neighbor coor, value: vector of pair of original vias coor and type (integer)
			auto it_adj = vias_adj_coor.find(vias_u);

			if(it_adj == vias_adj_coor.end()) { //upper neighbor not known as valid earlier
				vector<pair<Coordinates, int>> original; //empty vector
				// can insert key to second map (valid neighbor coor, value: empty vector)
				it_adj = vias_adj_coor.insert(make_pair(vias_u, original)).first; 
			}
			// insert value to second map (vector of pair of original vias coor and type (integer))
			// (valid earlier or not doesn't matter)
			it_adj->second.push_back(make_pair(it->first, 1));
		}

		//Do the same for right, down and left
		auto it_r = vias_org_type.find(vias_r);
		if(it_r == vias_org_type.end()) { 
			it->second.push_back(2); 
			auto it_adj = vias_adj_coor.find(vias_r);
			if(it_adj == vias_adj_coor.end()) { 
				vector<pair<Coordinates, int>> original; 
				it_adj = vias_adj_coor.insert(make_pair(vias_r, original)).first; 
			}
			it_adj->second.push_back(make_pair(it->first, 2));
		}

		//down
		auto it_d = vias_org_type.find(vias_d);
		if(it_d == vias_org_type.end()) { 
			it->second.push_back(3); 
			auto it_adj = vias_adj_coor.find(vias_d);
			if(it_adj == vias_adj_coor.end()) { 
				vector<pair<Coordinates, int>> original; 
				it_adj = vias_adj_coor.insert(make_pair(vias_d, original)).first; 
			}
			it_adj->second.push_back(make_pair(it->first, 3));
		}

		//left
		auto it_l = vias_org_type.find(vias_l);
		if(it_l == vias_org_type.end()) { 
			it->second.push_back(4); 
			auto it_adj = vias_adj_coor.find(vias_l);
			if(it_adj == vias_adj_coor.end()) { 
				vector<pair<Coordinates, int>> original;
				it_adj = vias_adj_coor.insert(make_pair(vias_l, original)).first; 
			}
			it_adj->second.push_back(make_pair(it->first, 4));
		}
	}
	

	//////////// generate the output file /////////////
	fstream fout;
	fout.open(argv[2],ios::out);

	fout << "Maximize" << endl << "  ";
	for (auto it = vias_org_type.begin(); it != vias_org_type.end(); it++) {
		for (auto ite = it->second.begin(); ite != it->second.end(); ite++) {			
			if (it != vias_org_type.begin() || ite != it->second.begin()) {
				fout << " + ";	 
			}
			fout << "R_" << it->first.x << "_" << it->first.y << "_" << *ite;			
		}
	}
	
	fout << endl << "Subject To" << endl;
	//first map ===== vias_org_type 
	//key: original vias coor
	//value: vector of valid neighbor types (integer) 
	for (auto it = vias_org_type.begin(); it != vias_org_type.end(); it++) {
		fout << "  R_" << it->first.x << "_" << it->first.y << ": ";
		for (auto ite = it->second.begin(); ite != it->second.end(); ite++) {			
			if (ite != it->second.begin()) {
				fout << " + ";	 
			}
			fout << "R_" << it->first.x << "_" << it->first.y << "_" << *ite;			
		}
		fout << " <= 1" << endl;
	}

	//second map ===== vias_adj_coor 
	//key: legitimate neighbor coor
	//value: vector of pair of original vias coor and type (integer)
	for (auto it = vias_adj_coor.begin(); it != vias_adj_coor.end(); it++) {
		fout << "  C_" << it->first.x << "_" << it->first.y << ": ";
		for (auto ite = it->second.begin(); ite != it->second.end(); ite++) {			
			if (ite != it->second.begin()) {
				fout << " + ";	 
			}
			fout << "R_" << ite->first.x << "_" << ite->first.y << "_" << ite->second;			
		}
		fout << " <= 1" << endl;
	}

	fout << "Bounds" << endl;

	fout << "Binary" << endl << "  ";
	for (auto it = vias_org_type.begin(); it != vias_org_type.end(); it++) {
		for (auto ite = it->second.begin(); ite != it->second.end(); ite++) {			
			if (it != vias_org_type.begin() || ite != it->second.begin()) {
				fout << " ";	 
			}
			fout << "R_" << it->first.x << "_" << it->first.y << "_" << *ite;			
		}
	}	

	fout << endl << "End" << endl;
	fout.close();

	return 0;
}
