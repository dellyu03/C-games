#include "pch.h"
#include "commands.h"  // 추가된 헤더 파일 포함
#include <winsock2.h>
#include <iostream>
#include <vector>
#include <ctime>
#include <string>
#include <algorithm>  // std::remove 사용을 위한 헤더

#pragma comment(lib, "ws2_32.lib")

#define SERVER_PORT 9000
#define BUF_SIZE 512

class CServer {
public:
    CServer();
    ~CServer();
    bool Initialize();
    void StartListening();
    void ReceiveCommandsFromClients();  // 명령어 수신 함수 추가

private:
    SOCKET m_listenSocket;
    std::vector<SOCKET> m_clientSockets;
};

CServer::CServer() : m_listenSocket(INVALID_SOCKET) {}

CServer::~CServer() {
    if (m_listenSocket != INVALID_SOCKET) {
        closesocket(m_listenSocket);
    }
    for (auto& clientSocket : m_clientSockets) {
        closesocket(clientSocket);
    }
    WSACleanup();
}

bool CServer::Initialize() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed!" << std::endl;
        return false;
    }

    m_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_listenSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed!" << std::endl;
        return false;
    }

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(m_listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed!" << std::endl;
        return false;
    }

    return true;
}

void CServer::StartListening() {
    if (listen(m_listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed!" << std::endl;
        return;
    }

    std::cout << "Server is listening on port " << SERVER_PORT << "..." << std::endl;

    while (true) {
        SOCKET clientSocket = accept(m_listenSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Accept failed!" << std::endl;
            continue;
        }

        m_clientSockets.push_back(clientSocket);
        std::cout << "Client connected!" << std::endl;

        ReceiveCommandsFromClients();  // 명령어 처리 함수 호출
    }
}

void CServer::ReceiveCommandsFromClients() {
    char buffer[BUF_SIZE];

    // 모든 클라이언트로부터 명령어를 받음
    for (auto& clientSocket : m_clientSockets) {
        int bytesReceived = recv(clientSocket, buffer, BUF_SIZE, 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            std::cout << "Received command from client: " << buffer << std::endl;

            // 클라이언트로부터 명령어가 "start"이면 랜덤 번호 전송
            if (strcmp(buffer, "start") == 0) {
                HandleStartCommand(clientSocket, m_clientSockets);
            }
            // 클라이언트로부터 명령어가 "list"이면 클라이언트 정보 출력
            else if (strcmp(buffer, "list") == 0) {
                HandleListCommand(clientSocket, m_clientSockets);
            }
            // 클라이언트로부터 명령어가 "exit"이면 해당 클라이언트 종료
            else if (strcmp(buffer, "exit") == 0) {
                HandleExitCommand(clientSocket, m_clientSockets);
            }
        }
        else {
            std::cerr << "Failed to receive data from client!" << std::endl;
        }
    }
}

int main() {
    CServer server;

    if (server.Initialize()) {
        server.StartListening();
    }

    return 0;
}
