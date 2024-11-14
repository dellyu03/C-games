#include "commands.h"
#include <iostream>
#include <ctime>
#include <cstdlib>
#include <sstream>  // ostringstream ����� ���� ���

#define BUF_SIZE 512

// "start" ��ɾ� ó�� �Լ�
void HandleStartCommand(SOCKET clientSocket, const std::vector<SOCKET>& clientSockets) {
    srand(static_cast<unsigned int>(time(0)));
    int randomNumber = rand() % 20 + 1;

    std::string message = "Random Number: " + std::to_string(randomNumber);
    send(clientSocket, message.c_str(), message.length(), 0);

    std::cout << "Sent random number to client: " << randomNumber << std::endl;
}

// "list" ��ɾ� ó�� �Լ�
void HandleListCommand(SOCKET clientSocket, const std::vector<SOCKET>& clientSockets) {
    std::string clientInfo = "Current number of clients: " + std::to_string(clientSockets.size()) + "\n";
    for (size_t i = 0; i < clientSockets.size(); ++i) {
        // SOCKET�� ���ڿ��� ��ȯ�� �� to_string�� ����ϴ� ��� ostringstream�� ���
        std::ostringstream oss;
        oss << clientSockets[i];
        clientInfo += "Client " + std::to_string(i + 1) + ": Socket ID: " + oss.str() + "\n";
    }

    send(clientSocket, clientInfo.c_str(), clientInfo.length(), 0);
    std::cout << "Sent client info to client." << std::endl;
}

// "exit" ��ɾ� ó�� �Լ�
void HandleExitCommand(SOCKET clientSocket, std::vector<SOCKET>& clientSockets) {
    std::cout << "Client requested exit." << std::endl;
    closesocket(clientSocket);
    clientSockets.erase(std::remove(clientSockets.begin(), clientSockets.end(), clientSocket), clientSockets.end());
    std::cout << "Client disconnected." << std::endl;
}
