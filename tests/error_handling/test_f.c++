#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
using namespace std;

int main(int argc, char** argv){
	ifstream infile(argv[1], ifstream::in);
	string line;
	if (!infile.is_open())
		return 1;
	while (getline(infile, line))
	{
    	istringstream iss(line);
	    int a, b;
		if (!(iss >> a >> b)) { break; } // error
		// process pair (a,b)
		cout << a << " " << b << endl;
	}
	return 0;
}
