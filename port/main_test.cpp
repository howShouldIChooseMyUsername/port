//#include <winsock2.h>
//
//#pragma comment(lib, "ws2_32.lib") // 라이브러리 링크 확인
//
//#include <iostream>
//#include "ip.hpp"
//
//using namespace std;
//
//int main() {
//	srand(time(nullptr));
//	WSADATA wsaData;
//	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
//		cout << "WSAStartup 실패: " << WSAGetLastError() << endl;
//		return 0;
//	}
//	NatType nat = ip::check_nat_type();
//	if (nat == NatType::Firewall) {
//		cout << "방화벽에 막혔습니다" << endl;
//	}
//	else if (nat == NatType::Symmetric) {
//		cout << "공유기가 대칭형입니다" << endl;
//	}
//	else if (nat == NatType::Restricted) {
//		cout << "공유기가 대칭형은 아닌데 풀콘은 아니네요 ㅠ" << endl;
//	}
//	else if (nat == NatType::FullCone) {
//		cout << "공유기가 풀콘입니다! 존나 야르!" << endl;
//	}
//	else {
//		cout << "에러났잖아 시발" << endl;
//	}
//}