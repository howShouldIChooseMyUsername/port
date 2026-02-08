
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#include "ip.hpp"

#include <vector>
#include <iostream>
#include <string>
#include <string_view>
#include <cstring>
#pragma pack(push, 1)
struct StunHeader {
	unsigned short type;
	unsigned short length;
	unsigned int magic;
	unsigned char tsid[12];
};
#pragma pack(pop)

#pragma pack(push, 1)
struct StunAttribute {
	unsigned short type;
	unsigned short length;
	unsigned int value;
};
#pragma pack(pop)
namespace NetworkConfig {
	constexpr int CHANGE_IP = 0x00000004;
	constexpr int CHANGE_PORT = 0x00000002;
	constexpr int CHANGE_REQUEST = 0x0003;
}

PublicAddr query_stun_server(SOCKET sock, const std::string_view stunServerUrl, const std::string_view port) {
	DWORD timeout = 1000;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char*>(&timeout), sizeof(timeout));

	struct addrinfo hints, * res;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	if (getaddrinfo(stunServerUrl.data(), port.data(), &hints, &res) != 0) {
		return { "0.0.0.0", 0 };
	}

	StunHeader req;
	req.type = htons(0x0001);
	req.length = 0;
	req.magic = htonl(0x2112A442);
	for (int i = 0; i < 12; i++) req.tsid[i] = rand() % 256; // 매직넘버는 랜덤으로

	sendto(sock, reinterpret_cast<char*>(&req), sizeof(req), 0, res->ai_addr, static_cast<int>(res->ai_addrlen));
	freeaddrinfo(res);

	char buf[512];
	struct sockaddr_in from_addr;
	int remote_len = sizeof(from_addr);
	int len = recvfrom(sock, buf, sizeof(buf), 0, reinterpret_cast<sockaddr*>(&from_addr), &remote_len);

	PublicAddr addr = { "0.0.0.0", 0 };
	if (len > 20) {
		int pos = 20;
		while (pos < len) {
			unsigned short attr_type = ntohs(*reinterpret_cast<const unsigned short*>(&buf[pos]));
			unsigned short attr_len = ntohs(*reinterpret_cast<const unsigned short*>(&buf[pos + 2]));

			if (attr_type == 0x0001) {
				addr.port = ntohs(*reinterpret_cast<unsigned short*>(&buf[pos + 6]));
				unsigned char* ip_ptr = reinterpret_cast<unsigned char*>(&buf[pos + 8]);
				char public_ip[INET_ADDRSTRLEN];
				sprintf_s(public_ip, "%d.%d.%d.%d", ip_ptr[0], ip_ptr[1], ip_ptr[2], ip_ptr[3]);
				addr.ip = public_ip;
				break;
			}
			else if (attr_type == 0x0020) {
				unsigned short xored_port = *reinterpret_cast<unsigned short*>(&buf[pos + 6]);
				addr.port = ntohs(xored_port ^ 0x2112A442);

				unsigned int xored_ip = *reinterpret_cast<unsigned int*>(&buf[pos + 8]);
				unsigned int real_ip = ntohs(xored_ip ^ 0x2112A442);

				struct in_addr in_addr_struct;
				in_addr_struct.S_un.S_addr = htonl(real_ip);
				char public_ip[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, &in_addr_struct, public_ip, sizeof(public_ip));
				addr.ip = public_ip;
				break;
			}
			pos += 4 + ((attr_len + 3) & ~3);
		}
	}
	return addr;
}

PublicAddr stun_with_change_request(SOCKET sock) {
	DWORD timeout = 1000;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char*>(&timeout), sizeof(timeout));
	struct addrinfo hints, * res;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	// DNS 확인
	if (getaddrinfo("stun.stunprotocol.org", "3478", &hints, &res) != 0) {
		return { "0.0.0.0", 0 };
	}

	StunHeader req;
	req.type = htons(0x0001);
	req.length = htons(sizeof(StunAttribute));
	req.magic = htonl(0x2112A442);
	for (int i = 0; i < 12; i++) req.tsid[i] = rand() % 256;

	StunAttribute attr;
	attr.type = htons(NetworkConfig::CHANGE_REQUEST);
	attr.length = htons(0x0004);
	attr.value = htonl(NetworkConfig::CHANGE_IP | NetworkConfig::CHANGE_PORT);
	
	char send_buf[sizeof(StunHeader) + sizeof(StunAttribute)];
	std::memcpy(send_buf, &req, sizeof(req));
	std::memcpy(send_buf + sizeof(req), &attr, sizeof(attr));

	sendto(sock, send_buf, sizeof(send_buf), 0, res->ai_addr, static_cast<int>(res->ai_addrlen));
	freeaddrinfo(res);

	char buf[512];
	struct sockaddr_in from_addr;
	int remote_len = sizeof(from_addr);
	int len = recvfrom(sock, buf, sizeof(buf), 0, reinterpret_cast<struct sockaddr*>(&from_addr), &remote_len);

	PublicAddr addr = { "0.0.0.0", 0 };
	if (len > 20) {
		int pos = 20;
		while (pos < len) {
			unsigned short attr_type = ntohs(*reinterpret_cast<const unsigned short*>(&buf[pos]));
			unsigned short attr_len = ntohs(*reinterpret_cast<const unsigned short*>(&buf[pos + 2]));

			if (attr_type == 0x0001) {
				addr.port = ntohs(*reinterpret_cast<unsigned short*>(&buf[pos + 6]));
				unsigned char* ip_ptr = reinterpret_cast<unsigned char*>(&buf[pos + 8]);
				char public_ip[INET_ADDRSTRLEN];
				sprintf_s(public_ip, "%d.%d.%d.%d", ip_ptr[0], ip_ptr[1], ip_ptr[2], ip_ptr[3]);
				addr.ip = public_ip;
				break;
			}
			else if (attr_type == 0x0020) { // XOR
				unsigned short xored_port = *reinterpret_cast<unsigned short*>(&buf[pos + 6]);
				addr.port = ntohs(xored_port ^ 0x2112);

				unsigned int xored_ip = *reinterpret_cast<unsigned int*>(&buf[pos + 8]);
				unsigned int real_ip = ntohl(xored_ip ^ 0x2112A442);
				
				struct in_addr in_addr_struct;
				in_addr_struct.S_un.S_addr = htonl(real_ip);
				char public_ip[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, &in_addr_struct, public_ip, sizeof(public_ip));
				addr.ip = public_ip;
				break;
			}
			pos += 4 + ((attr_len + 3) & ~3);
		}
	}
	return addr;
}

namespace ip {

	PublicAddr get_public_ip_udp() {
		SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		DWORD timeout = 1000;
		setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char*>(&timeout), sizeof(timeout));

		struct addrinfo hints, * res;
		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_DGRAM;

		// DNS 확인
		if (getaddrinfo("stun.l.google.com", "19302", &hints, &res) != 0) {
			closesocket(sock);
			return { "0.0.0.0", 0 };
		}

		StunHeader req;
		req.type = htons(0x0001);
		req.length = 0;
		req.magic = htonl(0x2112A442); 
		for (int i = 0; i < 12; i++) req.tsid[i] = rand() % 256;

		sendto(sock, reinterpret_cast<char*>(&req), sizeof(req), 0, res->ai_addr, static_cast<int>(res->ai_addrlen));
		freeaddrinfo(res);

		char buf[512];
		struct sockaddr_in from_addr;
		int remote_len = sizeof(from_addr);
		int len = recvfrom(sock, buf, sizeof(buf), 0, reinterpret_cast<struct sockaddr*>(&from_addr), &remote_len);

		PublicAddr addr = { "0.0.0.0", 0 };
		if (len > 20) {
			int pos = 20;
			while (pos < len) {
				unsigned short attr_type = ntohs(*reinterpret_cast<const unsigned short*>(&buf[pos]));
				unsigned short attr_len = ntohs(*reinterpret_cast<const unsigned short*>(&buf[pos + 2]));

				if (attr_type == 0x0001) { // MAPPED_ADDRESS
					addr.port = ntohs(*reinterpret_cast<unsigned short*>(&buf[pos + 6]));
					unsigned char* ip_ptr = reinterpret_cast<unsigned char*>(&buf[pos + 8]);
					char public_ip[INET_ADDRSTRLEN];
					sprintf_s(public_ip, "%d.%d.%d.%d", ip_ptr[0], ip_ptr[1], ip_ptr[2], ip_ptr[3]);
					addr.ip = public_ip;
					break;
				}
				else if (attr_type == 0x0020) { // XOR_MAPPED_ADDRESS
					unsigned short xored_port = *reinterpret_cast<unsigned short*>(&buf[pos + 6]);
					addr.port = ntohs(xored_port ^ 0x2112);

					unsigned int xored_ip = *reinterpret_cast<unsigned int*>(&buf[pos + 8]);
					unsigned int real_ip = ntohl(xored_ip ^ 0x2112A442); 

					struct in_addr in_addr_struct;
					in_addr_struct.S_un.S_addr = htonl(real_ip);
					char public_ip[INET_ADDRSTRLEN];
					inet_ntop(AF_INET, &in_addr_struct, public_ip, sizeof(public_ip));
					addr.ip = public_ip;
					break;
				}
				pos += 4 + ((attr_len + 3) & ~3);
			}
		}

		closesocket(sock);
		return addr;
	}

	PublicAddr get_public_ip_udp_with_socket(SOCKET sock) {
		DWORD timeout = 1000;
		setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char*>(&timeout), sizeof(timeout));
		struct addrinfo hints, * res;
		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_DGRAM;

		// DNS 확인
		if (getaddrinfo("stun.l.google.com", "19302", &hints, &res) != 0) {
			return { "0.0.0.0", 0 };
		}

		StunHeader req;
		req.type = htons(0x0001);
		req.length = 0;
		req.magic = htonl(0x2112A442);
		for (int i = 0; i < 12; i++) req.tsid[i] = rand() % 256;

		sendto(sock, reinterpret_cast<char*>(&req), sizeof(req), 0, res->ai_addr, static_cast<int>(res->ai_addrlen));
		freeaddrinfo(res);

		char buf[512];
		struct sockaddr_in from_addr;
		int remote_len = sizeof(from_addr);
		int len = recvfrom(sock, buf, sizeof(buf), 0, reinterpret_cast<struct sockaddr*>(&from_addr), &remote_len);

		PublicAddr addr = { "0.0.0.0", 0 };
		if (len > 20) {
			int pos = 20;
			while (pos < len) {
				unsigned short attr_type = ntohs(*reinterpret_cast<const unsigned short*>(&buf[pos]));
				unsigned short attr_len = ntohs(*reinterpret_cast<const unsigned short*>(&buf[pos + 2]));

				if (attr_type == 0x0001) { // MAPPED_ADDRESS
					addr.port = ntohs(*reinterpret_cast<unsigned short*>(&buf[pos + 6]));
					unsigned char* ip_ptr = reinterpret_cast<unsigned char*>(&buf[pos + 8]);
					char public_ip[INET_ADDRSTRLEN];
					sprintf_s(public_ip, "%d.%d.%d.%d", ip_ptr[0], ip_ptr[1], ip_ptr[2], ip_ptr[3]);
					addr.ip = public_ip;
					break;
				}
				else if (attr_type == 0x0020) { // XOR_MAPPED_ADDRESS
					unsigned short xored_port = *reinterpret_cast<unsigned short*>(&buf[pos + 6]);
					addr.port = ntohs(xored_port ^ 0x2112);

					unsigned int xored_ip = *reinterpret_cast<unsigned int*>(&buf[pos + 8]);
					unsigned int real_ip = ntohl(xored_ip ^ 0x2112A442);

					struct in_addr in_addr_struct;
					in_addr_struct.S_un.S_addr = htonl(real_ip);
					char public_ip[INET_ADDRSTRLEN];
					inet_ntop(AF_INET, &in_addr_struct, public_ip, sizeof(public_ip));
					addr.ip = public_ip;
					break;
				}
				pos += 4 + ((attr_len + 3) & ~3);
			}
		}
		return addr;
	}
	
	NatType check_nat_type() {
		try {
			SOCKET testSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

			PublicAddr googleStun = query_stun_server(testSocket, "stun.l.google.com", "19302");
			Sleep(1000);
			PublicAddr stunChangeRequest = stun_with_change_request(testSocket);
			Sleep(1000);
			PublicAddr cloudflareStun = query_stun_server(testSocket, "stun.cloudflare.com", "3478");
			Sleep(1000);
			PublicAddr twilioStun = query_stun_server(testSocket, "global.stun.twilio.com", "3478");
			if (!(googleStun.ip != "0.0.0.0")) {
				return NatType::Firewall;
			}
			else {
				if (googleStun.port != 0 && cloudflareStun.port != 0 && twilioStun.port != 0) {
					if (!(googleStun.port == cloudflareStun.port && cloudflareStun.port == twilioStun.port)) {
						return NatType::Symmetric;
					}
					else { // 여기에 온 순간부터 stunChangeRequest를 제외한 모든 애들은 사실상 같다고(equals, ==) 봐도 됨
						if (stunChangeRequest.ip != "0.0.0.0") {
							return NatType::FullCone;
						}
						else {
							return NatType::Restricted;
						}
					}
				}
				else {
					if (twilioStun.port == 0 && cloudflareStun.port == 0) {
						if (stunChangeRequest.ip != "0.0.0.0") {
							return NatType::FullCone;
						}
						else {
							return NatType::Restricted;
						}
					}
					else if (cloudflareStun.port == 0) {
						if (!(googleStun.port == twilioStun.port)) {
							return NatType::Symmetric;
						}
						else {
							if (stunChangeRequest.ip != "0.0.0.0") {
								return NatType::FullCone;
							}
							else {
								return NatType::Restricted;
							}
						}
					}
					else if (twilioStun.port == 0) {
						if (!(googleStun.port == cloudflareStun.port)) {
							return NatType::Symmetric;
						}
						else {
							if (stunChangeRequest.ip != "0.0.0.0") {
								return NatType::FullCone;
							}
							else {
								return NatType::Restricted;
							}
						}
					}
					else {
						return NatType::INVALID;
					}
				}
			}
		}
		catch (...) {
			return NatType::INVALID;
		}
	}
}