#include "strutils.hpp"

#include <iostream>

using namespace std;

namespace strutils {

	vector<string> split_string(const string& str, char identifier) {
		string temp = "";
		vector<string> arr;

		for (char c : str) {
			if (c == identifier) {
				arr.push_back(temp);
				temp = "";
				continue;
			}
			// not
			temp += c;
		}
		arr.push_back(temp);

		return arr;
	}

	string merge_string(const vector<string>& vec, string padding) {
		string temp = "";
		for (string s : vec) {
			temp += s;
			temp += padding;
		}
		return temp;
	}

}