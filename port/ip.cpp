
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
        DWORD timeout = 1000;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char*>(&timeout), sizeof(timeout));

        struct addrinfo hints, * res;
        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;

        // DNS 확인
        if (getaddrinfo("stun.l.google.com", "19302", &hints, &res) != 0) {
            std::cout << "failed to fetch getaddrinfo" << std::endl;
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
            std::cout << "failed to fetch getaddrinfo" << std::endl;
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
    
}