
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#ifndef SIO_UDP_CONNRESET
#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR, 12)
#endif

#include <iostream>
#include <vector>
#include <future>
#include "port.hpp"

// 절대 여기에서 WSAStartup(), WSACleanup() 하지않기

// 공통 int형 리턴 값 별 해석
// 0 <- 이미 쓰는중
// 1 <- 사용 가능
// -1 <- 에러
namespace port {
	// NO LISTEN
	int check_port_tcp(unsigned short port) {
		// 소켓 생성
		SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (s == INVALID_SOCKET) return -1;

		// addr 생성과 초기화
		struct sockaddr_in addr;
		ZeroMemory(&addr, sizeof(addr));

		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

		// 실패했을때
		if (bind(s, reinterpret_cast<const struct sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
			if (listen(s, SOMAXCONN) == SOCKET_ERROR) {
				int err = WSAGetLastError();
				closesocket(s);
				return (err == WSAEADDRINUSE) ? 0 : -1;
			}
			closesocket(s);
			return 1;
		} // 성공!
		else {
			closesocket(s);
			return 1;
		}
	};

	int check_port_udp(unsigned short port) {
		// 소켓 생성
		SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (s == INVALID_SOCKET)  return -1;

		// addr 생성과 초기화
		struct sockaddr_in addr;
		ZeroMemory(&addr, sizeof(addr));

		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

		// 실패했을때
		if (bind(s, reinterpret_cast<const struct sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
			int err = WSAGetLastError();
			closesocket(s);
			return (err == WSAEADDRINUSE) ? 0 : -1;
		} // 성공!
		else {
			closesocket(s);
			return 1;
		}
	}

	int open_tcp_port(unsigned short port, SOCKET* outListenSock) {
		// 소켓 생성
		SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (s == INVALID_SOCKET) return -1;

		// addr 생성과 초기화
		struct sockaddr_in addr;
		ZeroMemory(&addr, sizeof(addr));

		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
		addr.sin_port = htons(port);	

		// 실패했을때
		if (bind(s, reinterpret_cast<const struct sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
			int err = WSAGetLastError();
			closesocket(s);
			return (err == WSAEADDRINUSE) ? 0 : -1;
		}

		// 성공!
		*outListenSock = s;
		return 1;
	}

	int open_udp_port(unsigned short port, SOCKET* outListenSock) {
		// 소켓 생성
		SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (s == INVALID_SOCKET) return -1;

		// addr 생성과 초기화
		struct sockaddr_in addr;
		ZeroMemory(&addr, sizeof(addr));

		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
		addr.sin_port = htons(port);

		// 실패했을때
		if (bind(s, reinterpret_cast<const struct sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
			int err = WSAGetLastError();
			closesocket(s);
			return (err == WSAEADDRINUSE) ? 0 : -1;
		}

		// 성공!
		*outListenSock = s;
		return 1;
	}

	int create_send_sock_udp(unsigned short port, SOCKET* sock) {
		// 소켓 생성
		SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (s == INVALID_SOCKET) return -1;

		// addr 생성과 초기화
		struct sockaddr_in addr;
		ZeroMemory(&addr, sizeof(addr));

		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
		addr.sin_port = htons(port);

		// 실패했을때
		if (bind(s, reinterpret_cast<const struct sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
			int err = WSAGetLastError();
			closesocket(s);
			return (err == WSAEADDRINUSE) ? 0 : -1;
		} // 성공!
		else {
			*sock = s;
			return 1;
		}
	}

	void create_server_sock_udp(unsigned short port, sockaddr_in* server) {
		// addr 생성과 초기화
		struct sockaddr_in addr = *server;

		addr.sin_family = AF_INET;
		inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
		addr.sin_port = htons(port);

		*server = addr;
	}

	bool check_target_port_fast_tcp(std::string ip, unsigned short port, int timeout_ms) {
		SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		unsigned long mode = 1;
		ioctlsocket(sock, FIONBIO, &mode);

		struct sockaddr_in addr;
		ZeroMemory(&addr, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

		connect(sock, reinterpret_cast<const struct sockaddr*>(&addr), sizeof(addr));

		fd_set write_fds;
		FD_ZERO(&write_fds);
		FD_SET(sock, &write_fds);

		timeval tv;
		tv.tv_sec = timeout_ms / 1000;
		tv.tv_usec = (timeout_ms % 1000) * 1000;

		int res = select(0, NULL, &write_fds, NULL, &tv);

		bool is_open = (res > 0);

		closesocket(sock);
		return is_open;
	}

	std::vector<unsigned short> scan_target_ports_tcp(std::string ip, const std::vector<unsigned short>& ports) {
		const short max_concurrent = 50;
		std::vector<std::future<std::pair<unsigned short, bool>>> futures;
		std::vector<unsigned short> portList;
		for (size_t i = 0; i < ports.size(); i++) {
			futures.push_back(std::async(std::launch::async, [ip, port = ports[i]]() {
				return std::make_pair(port, port::check_target_port_fast_tcp(ip, port, 1000));
				}));

			if (futures.size() >= max_concurrent || i == ports.size() - 1) {
				for (auto& f : futures) {
					auto result = f.get();
					if (result.second) {
						portList.push_back(result.first);
					}
				}
				futures.clear();
			}
		}
		std::cout << "포트 스캐닝 끝" << std::endl;
		return portList;
	}

	bool check_target_port_fast_udp(std::string ip, unsigned short port, int timeout_ms) {
		SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

		unsigned long mode = 1;
		ioctlsocket(sock, FIONBIO, &mode);

		DWORD dwBytesReturned = 0;
		BOOL bNewBeHavior = TRUE;
		
		WSAIoctl(sock, SIO_UDP_CONNRESET, &bNewBeHavior, sizeof(bNewBeHavior), NULL, 0, &dwBytesReturned, NULL, NULL);

		struct sockaddr_in addr;
		ZeroMemory(&addr, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

		const std::string dummy = "ping";
		sendto(sock, dummy.c_str(), static_cast<int>(dummy.length()), 0, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));

		fd_set read_fds;
		FD_ZERO(&read_fds);
		FD_SET(sock, &read_fds);

		timeval tv;
		tv.tv_sec = timeout_ms / 1000;
		tv.tv_usec = (timeout_ms % 1000) * 1000;

		int res = select(0, &read_fds, NULL, NULL, &tv);

		bool is_open = (res == 0);
		closesocket(sock);

		return is_open;
	}

	void scan_target_ports_udp(std::string ip, const std::vector<unsigned short>& ports) {
		const short max_concurrent = 50;
		std::vector<std::future<std::pair<unsigned short, bool>>> futures;

		for (size_t i = 0; i < ports.size(); i++) {
			futures.push_back(std::async(std::launch::async, [ip, port = ports[i]]() {
				return std::make_pair(port, port::check_target_port_fast_udp(ip, port, 1000));
				}));

			if (futures.size() >= max_concurrent || i == ports.size() - 1) {
				for (auto& f : futures) {
					auto result = f.get();
					if (result.second) {
						std::cout << "[?] UDP Port " << result.first << " is OPEN (or Filtered)" << std::endl;
					}
				}
				futures.clear();
			}
		}
	}
}