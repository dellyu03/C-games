#include "commands.h"
#include <iostream>
#include <ctime>
#include <cstdlib>
#include <sstream>  // ostringstream 사용을 위한 헤더

#define BUF_SIZE 512

// "start" 명령어 처리 함수
void HandleStartCommand(SOCKET clientSocket, const std::vector<SOCKET>& clientSockets) {
    srand(static_cast<unsigned int>(time(0)));
    int randomNumber = rand() % 20 + 1;

    std::string message = "Random Number: " + std::to_string(randomNumber);
    send(clientSocket, message.c_str(), message.length(), 0);

    std::cout << "Sent random number to client: " << randomNumber << std::endl;
}

// "list" 명령어 처리 함수
void HandleListCommand(SOCKET clientSocket, const std::vector<SOCKET>& clientSockets) {
    std::string clientInfo = "Current number of clients: " + std::to_string(clientSockets.size()) + "\n";
    for (size_t i = 0; i < clientSockets.size(); ++i) {
        // SOCKET을 문자열로 변환할 때 to_string을 사용하는 대신 ostringstream을 사용
        std::ostringstream oss;
        oss << clientSockets[i];
        clientInfo += "Client " + std::to_string(i + 1) + ": Socket ID: " + oss.str() + "\n";
    }

    send(clientSocket, clientInfo.c_str(), clientInfo.length(), 0);
    std::cout << "Sent client info to client." << std::endl;
}

// "exit" 명령어 처리 함수
void HandleExitCommand(SOCKET clientSocket, std::vector<SOCKET>& clientSockets) {
    std::cout << "Client requested exit." << std::endl;
    closesocket(clientSocket);
    clientSockets.erase(std::remove(clientSockets.begin(), clientSockets.end(), clientSocket), clientSockets.end());
    std::cout << "Client disconnected." << std::endl;
}
