#pragma once

#include <winsock2.h>
#include <vector>
#include <string>
#pragma comment(lib, "ws2_32.lib")

struct PublicAddr {
	std::string ip;
	unsigned short port;
};

enum class NatType {
	Symmetric,  // 대칭형, 제일 까다로움(나갈때마다 포트가 바뀜)
	Restricted, // 포트는 같지만, 제 3의 IP 응답 거부
	FullCone,   // 제일 널널함, 포트도 같고, 제 3의 IP 응답도 수용
	Firewall,   // 방화벽에 막힌 케이스, 응답도 안올때 사용
	INVALID     // 에러가 발생했을때 사용
};

namespace ip {
	
	PublicAddr get_public_ip_udp();
	PublicAddr get_public_ip_udp_with_socket(SOCKET sock);

	NatType check_nat_type(SOCKET sock);

}