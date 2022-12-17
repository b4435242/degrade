#ifndef _RECORD_
#define _RECORD_

#include <vector>
#include <unordered_map>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <ctime>
#include <regex>

#include "rapidjson/document.h" 
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h" 

using namespace std;

string loc_to_string(vector<int> &loc);
string get_base_type(string type);
vector<int> get_arr_size(string type);


class Record {
public:
	Record(vector<int>& _loc): loc(_loc){}

	void write_to_json();


	template<typename T>
	void dump_val(char* name, T val);
	template<typename T>
	void dump_arr(char* name, T ptr, vector<int> &sizes);
	template<typename T>
	void dump_2d_arr(char* name, T ptr, vector<int> &sizes);


	// read manual cheatsheet of len id of ptr 
	static void read_lenIdOfPtr();
	vector<int> get_ptr_size(char*);
		
private:
	rapidjson::Document doc;
	rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
	vector<int> loc;
	// In order to dump data of pointer
	// pos -> var -> id of len
public:
	static bool lenIdOfPtr_flag;
	unordered_map<string, vector<int>> len2val;	
	static unordered_map<string, unordered_map<string, vector<string>>> lenIdOfPtr;

};

//
// API
// typename -> var_id -> val
//
void DG_RECORD(int beginLine, int beginCol, int endLine, int endCol, char** na
mes, char** types, void** vals, int size);

#endif
