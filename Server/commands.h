
#pragma once
#include <winsock2.h>
#include <vector>
#include <string>

// ��ɾ� ó�� �Լ� ����
void HandleStartCommand(SOCKET clientSocket, const std::vector<SOCKET>& clientSockets);
void HandleListCommand(SOCKET clientSocket, const std::vector<SOCKET>& clientSockets);
void HandleExitCommand(SOCKET clientSocket, std::vector<SOCKET>& clientSockets);