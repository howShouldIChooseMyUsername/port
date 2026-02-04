#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <windows.h>

#include <natupnp.h>
#include <comutil.h>
#include <comdef.h>
#include <atlbase.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "comsuppw.lib")

#include "upnp.hpp"

#include <iostream>
#include <string>
#include <cstdlib>

namespace upnp {

	std::string get_local_ip() {
		ULONG outBufLen = 15000;
		PIP_ADAPTER_ADDRESSES pAddresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);  // 아오 말록 확그냥 이씨

		if (GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST, nullptr, pAddresses, &outBufLen) != ERROR_SUCCESS) {
			free(pAddresses);
			return "";
		}

		std::string realIp = "";
		for (PIP_ADAPTER_ADDRESSES pCurrAddresses = pAddresses; pCurrAddresses != nullptr; pCurrAddresses = pCurrAddresses->Next) {
			// 가상 어댑터가 아닌지 확인
			if (pCurrAddresses->IfType != IF_TYPE_ETHERNET_CSMACD && pCurrAddresses->IfType != IF_TYPE_IEEE80211)
				continue;

			// 현재 어댑터가 작동중인지 확인
			if (pCurrAddresses->OperStatus != IfOperStatusUp)
				continue;

			std::wstring desc = pCurrAddresses->Description;
			if (desc.find(L"Virtual") != std::wstring::npos ||
				desc.find(L"VPN") != std::wstring::npos ||
				desc.find(L"Vpn") != std::wstring::npos ||
				desc.find(L"vpn") != std::wstring::npos ||
				desc.find(L"Radmin") != std::wstring::npos ||
				desc.find(L"radmin") != std::wstring::npos ||
				desc.find(L"RADMIN") != std::wstring::npos ||
				desc.find(L"Hamachi") != std::wstring::npos ||
				desc.find(L"hamachi") != std::wstring::npos ||
				desc.find(L"HAMACHI") != std::wstring::npos)
				continue;

			PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pCurrAddresses->FirstUnicastAddress;
			if (pUnicast != nullptr) {
				struct sockaddr_in* sa_in = reinterpret_cast<struct sockaddr_in*>(pUnicast->Address.lpSockaddr);
				char buf[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, &(sa_in->sin_addr), buf, sizeof(buf));
				realIp = buf;
				break;
			}
		}

		free(pAddresses);
		return realIp;
	}

	bool map_upnp_port(unsigned short port, std::string localIp) {
		// com 초기화
		HRESULT hr = CoInitialize(nullptr);
		if (FAILED(hr)) return false;
		bool success = false;

		/*IUPnPNAT* pNAT = nullptr;
		IStaticPortMappingCollection* pCollection = nullptr;
		IStaticPortMapping* pMapping = nullptr;*/
		{
			CComPtr<IUPnPNAT> pNAT;
			CComPtr<IStaticPortMappingCollection> pCollection;
			CComPtr<IStaticPortMapping> pMapping;

			// UPnP NAT 객체 생성
			hr = CoCreateInstance(CLSID_UPnPNAT, nullptr, CLSCTX_ALL, IID_IUPnPNAT, reinterpret_cast<void**>(&pNAT));
			if (SUCCEEDED(hr) && pNAT) {
				hr = pNAT->get_StaticPortMappingCollection(&pCollection);
				if (SUCCEEDED(hr) && pCollection) {
					_bstr_t bstrLocalIp(localIp.c_str());
					_bstr_t bstrProtocol("UDP");
					_bstr_t bstrDescription("UPnP");

					hr = pCollection->Add(port, bstrProtocol, port, bstrLocalIp, VARIANT_TRUE, bstrDescription, &pMapping);

					if (SUCCEEDED(hr)) {
						success = true;
					}
					else {
						std::cout << "UPnP Add 실패: 0x" << std::hex << hr << std::endl;
					}
				}
			}
		}
		/*if (pMapping) pMapping->Release();
		if (pCollection) pCollection->Release();
		if (pNAT) pNAT->Release();*/
		CoUninitialize();

		return success;
	}

	bool is_upnp_available() {
		HRESULT hr = CoInitialize(nullptr);
		if (FAILED(hr)) return false;

		IUPnPNAT* pNat = nullptr;
		IStaticPortMappingCollection* pCollection = nullptr;
		bool bSupported = false;

		hr = CoCreateInstance(CLSID_UPnPNAT, nullptr, CLSCTX_ALL, IID_IUPnPNAT, reinterpret_cast<void**>(&pNat));

		if (SUCCEEDED(hr) && pNat) {
			// UPnP 지원 안하면 pCollection은 nullptr이 됨
			hr = pNat->get_StaticPortMappingCollection(&pCollection);

			if (SUCCEEDED(hr) && pCollection != nullptr) {
				bSupported = true;
				pCollection->Release();
			}
			pNat->Release();
		}
		CoUninitialize();
		return bSupported;
	}	
}