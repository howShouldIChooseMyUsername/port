#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib") // 라이브러리 링크 확인

#include <iostream>
#include "port.hpp"

using namespace std;

int main() {
	srand(time(nullptr));
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		cout << "failed to initialize winsock" << endl;
		return 1;
	}

	float timeout_s = port::get_udp_timeout_sec();
	cout << "공유기 타임아웃 시간: " << timeout_s << endl;

	WSACleanup();
	return 0;
}