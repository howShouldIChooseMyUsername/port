#pragma once

#include <iostream>
#include <vector>

using namespace std;

namespace strutils {
	
	vector<string> split_string(const string& str, char identifier = ' ');

	string merge_string(const vector<string>& vec, string padding = "");

}