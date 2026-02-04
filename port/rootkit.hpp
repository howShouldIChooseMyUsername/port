
#pragma once

#include <iostream>
#include <variant>
#include <vector>

namespace rootkit {

	std::string run_cmd_with_output(const std::string& cmd);

	std::string to_cmd_script(const std::string& cmd);
	
	std::pair<std::string, std::variant<bool, nullptr_t>> exploit_bat_formatter(std::string& ip, unsigned short port,const std::string& str);

	std::vector<std::string> get_username_list();
}