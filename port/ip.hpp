#pragma once

#include <winsock2.h>
#include <vector>
#include <string>
#pragma comment(lib, "ws2_32.lib")

struct PublicAddr {
	std::string ip;
	unsigned short port;
};

namespace ip {
	PublicAddr get_public_ip_udp();
}