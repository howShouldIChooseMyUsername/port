#include "strutils.hpp"

#include <iostream>

namespace strutils {

	std::vector<std::string> split_string(const std::string& str, char identifier) {
		std::string temp = "";
		std::vector<std::string> arr;

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

	std::string merge_string(const std::vector<string>& vec, std::string padding) {
		string temp = "";
		for (string s : vec) {
			temp += s;
			temp += padding;
		}
		return temp;
	}

}