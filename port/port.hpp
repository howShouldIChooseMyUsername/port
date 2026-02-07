#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

namespace port {
	int check_port_tcp(unsigned short port);
	int check_port_udp(unsigned short port);

	int open_tcp_port(unsigned short port, SOCKET* outListenSock);
	int open_udp_port(unsigned short port, SOCKET* outListenSock);

	int create_send_sock_udp(unsigned short port, SOCKET* sock);
	void create_server_sock_udp(unsigned short port, sockaddr_in* server);

	bool check_target_port_fast_tcp(std::string ip, unsigned short port, int timeout_ms);
	std::vector<unsigned short> scan_target_ports_tcp(std::string ip, const std::vector<unsigned short>& ports);
	bool check_target_port_fast_udp(std::string ip, unsigned short port, int  timeout_ms);
	void scan_target_ports_udp(std::string ip, const std::vector<unsigned short>& ports);

	float get_udp_timeout_sec();
}
