
#pragma once
#include <winsock2.h>
#include <vector>
#include <string>

// 명령어 처리 함수 선언
void HandleStartCommand(SOCKET clientSocket, const std::vector<SOCKET>& clientSockets);
void HandleListCommand(SOCKET clientSocket, const std::vector<SOCKET>& clientSockets);
void HandleExitCommand(SOCKET clientSocket, std::vector<SOCKET>& clientSockets);