
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#include "ip.hpp"

#include <vector>
#include <iostream>
#include <string>
#pragma pack(push, 1)
struct StunHeader {
	unsigned short type;
	unsigned short length;
	unsigned int magic;
	unsigned char tsid[12];
};
#pragma pack(pop)

namespace ip {
	PublicAddr get_public_ip_udp() {
		SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		DWORD timeout = 1000; //ms
		setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char*>(&timeout), sizeof(timeout));

		struct sockaddr_in stun_addr;
		ZeroMemory(&stun_addr, sizeof(stun_addr));

		stun_addr.sin_family = AF_INET;
		stun_addr.sin_port = htons(19302);
		inet_pton(AF_INET, "74.125.250.129", &stun_addr.sin_addr);
		
		StunHeader req;
		req.type = htons(0x0001);
		req.length = 0;
		req.magic = htonl(0x2112A442);
		for (int i = 0; i < 12; i++) req.tsid[i] = rand() % 256;
		std::cout << "sizeof req: " << sizeof(req) << std::endl;
		sendto(sock, reinterpret_cast<char*>(&req), sizeof(req), 0, reinterpret_cast<const struct sockaddr*>(&stun_addr), sizeof(stun_addr));

		char buf[512];
		int remote_len = sizeof(stun_addr);
		int len = recvfrom(sock, buf, sizeof(buf), 0, reinterpret_cast<struct sockaddr*>(&stun_addr), &remote_len);
		if (len == SOCKET_ERROR) {
			std::cout << "timeout error: " << WSAGetLastError() << std::endl;
		}

		PublicAddr addr = { "0.0.0.0", 0 };
		if (len > 20) {
			int pos = 20;
			
			while (pos < len) {
				unsigned short attr_type = ntohs(*reinterpret_cast<const unsigned short*>(&buf[pos]));
				unsigned short attr_len = ntohs(*reinterpret_cast<const unsigned short*>(&buf[pos + 2]));

				if (attr_type == 0x0001) {
					// [Type 2B] + [Len 2B] + [Reserved 1B] + [Family 1B (0x01은 IPv4)] + [Port 2B] + [IP 4B]
					unsigned char* ip_ptr = reinterpret_cast<unsigned char*>(&buf[pos + 8]);
					char public_ip[16];
					sprintf_s(public_ip, "%d.%d.%d.%d", ip_ptr[0], ip_ptr[1], ip_ptr[2], ip_ptr[3]);
					break;
				}
				else if (attr_type == 0x0020) {
					unsigned int xored_ip = *reinterpret_cast<unsigned int*>(&buf[pos + 8]);	
					unsigned int real_ip = ntohl(xored_ip ^ 0x21122A442);

					struct in_addr addr;
					ZeroMemory(&addr, sizeof(addr));
					addr.S_un.S_addr = htonl(real_ip);
					char public_ip[INET_ADDRSTRLEN];
					inet_ntop(AF_INET, &addr, public_ip, INET_ADDRSTRLEN);
					break;
				}

				pos += 4 + ((attr_len + 3) & ~3);
			}
		}

		closesocket(sock);
		return addr;
	}
}