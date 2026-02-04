#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <winsock2.h>
#include <ws2tcpip.h>

#include <natupnp.h>
#include <comutil.h>
#include <atlbase.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

#include <string>

namespace upnp {
	
	std::string get_local_ip();

	bool map_upnp_port(unsigned short port, std::string localIp);

	bool is_upnp_available();
}