#include "pch.h"
#include <winsock2.h>
#include <iostream>
#include <vector>
#include <ctime>
#include <string>
#include <thread>
#include <algorithm>  // std::remove 사용을 위한 헤더
#include <random>
#include <chrono>     // 카운트다운을 위한 라이브러리

#pragma comment(lib, "ws2_32.lib")

#define SERVER_PORT 9000
#define BUF_SIZE 512

// 서버 구조체
class CServer {
public:
    CServer();
    ~CServer();
    bool Initialize();
    void StartListening();
    void SendRandomNumberToClient(SOCKET clientSocket);
    void DisplayClientInfo();
    void ReceiveCommandsFromClient(SOCKET clientSocket);
    void StartGame();
    void EndGame();
    void SendProcessComplete(SOCKET clientSocket);

private:
    SOCKET m_listenSocket;
    std::vector<SOCKET> m_clientSockets;
    int gameState;  // 0: 준비, 1: 게임 시작, 2: 게임 중
};

CServer::CServer() : m_listenSocket(INVALID_SOCKET), gameState(0) {}

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

        // 각 클라이언트마다 별도의 스레드로 명령어를 처리하도록 함
        std::thread clientThread(&CServer::ReceiveCommandsFromClient, this, clientSocket);
        clientThread.detach();  // 스레드를 분리하여 비동기로 실행
    }
}

void CServer::SendRandomNumberToClient(SOCKET clientSocket) {
    // 1부터 20까지의 숫자 중 홀수만 선택
    std::vector<int> numbers;
    for (int i = 1; i <= 20; ++i) {
        if (i % 2 != 0) {  // 홀수만 추가
            numbers.push_back(i);
        }
    }

    // 랜덤 셔플을 위한 랜덤 엔진과 분포
    std::random_device rd;
    std::mt19937 g(rd());  // Mersenne Twister 엔진을 사용
    std::shuffle(numbers.begin(), numbers.end(), g);  // 벡터 내 숫자 랜덤하게 섞기

    // 뽑을 숫자 개수는 8개로 제한
    numbers.resize(8);

    // 랜덤 홀수 리스트 전송
    std::string message = "number_list: ";
    for (int num : numbers) {
        message += std::to_string(num) + " ";
    }

    send(clientSocket, message.c_str(), message.length(), 0);
    std::cout << "Sent random odd numbers to client: " << message << std::endl;
}

void CServer::DisplayClientInfo() {
    std::cout << "Current number of clients: " << m_clientSockets.size() << std::endl;
    for (size_t i = 0; i < m_clientSockets.size(); ++i) {
        std::cout << "Client " << i + 1 << ": Socket ID: " << m_clientSockets[i] << std::endl;
    }
}

void CServer::SendProcessComplete(SOCKET clientSocket) {
    if (gameState == 2) {  // 게임 중 상태에서는 process_complete 전송하지 않음
        return;
    }

    std::string processCompleteMessage = "process_complete";
    send(clientSocket, processCompleteMessage.c_str(), processCompleteMessage.length(), 0);
    std::cout << "Sent 'process_complete' message to client." << std::endl;
}

void CServer::ReceiveCommandsFromClient(SOCKET clientSocket) {
    char buffer[BUF_SIZE];

    // 클라이언트로부터 명령어를 받을 때까지 기다림
    while (true) {
        int bytesReceived = recv(clientSocket, buffer, BUF_SIZE, 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            std::cout << "Received command from client: " << buffer << std::endl;

            // 클라이언트로부터 명령어가 "game_start"이면 랜덤 번호 전송
            if (strcmp(buffer, "game_start") == 0) {
                if (gameState == 1 || gameState == 2) {
                    std::string alreadyGameMessage = "Game already started or in progress.";
                    send(clientSocket, alreadyGameMessage.c_str(), alreadyGameMessage.length(), 0);
                    std::cout << "Sent 'Game already started' message to client." << std::endl;
                }
                else {
                    StartGame();  // 게임 시작
                }
            }
            // 클라이언트로부터 명령어가 "exit"이면 해당 클라이언트 종료
            else if (strcmp(buffer, "exit") == 0) {
                std::cout << "Client requested exit." << std::endl;
                closesocket(clientSocket);
                m_clientSockets.erase(std::remove(m_clientSockets.begin(), m_clientSockets.end(), clientSocket), m_clientSockets.end());
                std::cout << "Client disconnected." << std::endl;
                break; // 해당 클라이언트 스레드 종료
            }
            // 알 수 없는 명령어 처리
            else {
                std::string unknownMessage = "Unknown command";
                send(clientSocket, unknownMessage.c_str(), unknownMessage.length(), 0);
                std::cout << "Sent unknown command message to client." << std::endl;
            }

            // 명령어 처리 후 완료 메시지 전송 (게임 중 상태에서는 전송하지 않음)
            SendProcessComplete(clientSocket);
        }
        else {
            std::cerr << "Failed to receive data from client!" << std::endl;
            break; // 오류 발생 시 연결 종료
        }
    }
}

void CServer::StartGame() {
    if (gameState == 1 || gameState == 2) {
        std::cout << "Game already started or in progress!" << std::endl;
        return;  // 게임이 이미 시작되었거나 진행 중이면 다시 시작하지 않음
    }

    // 게임 상태를 "게임 시작"으로 변경
    gameState = 1;

    // 게임 시작을 표시
    std::cout << "Game started!" << std::endl;

    // 모든 클라이언트에게 게임 시작 메시지 전송
    std::string gameStartMessage = "game_start";
    for (SOCKET clientSocket : m_clientSockets) {
        send(clientSocket, gameStartMessage.c_str(), gameStartMessage.length(), 0);
    }

    // 10초 카운트다운
    for (int i = 10; i > 0; --i) {
        std::string countdownMessage = "Countdown: " + std::to_string(i) + " seconds";
        for (SOCKET clientSocket : m_clientSockets) {
            send(clientSocket, countdownMessage.c_str(), countdownMessage.length(), 0);
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // 랜덤 숫자 전송
    for (SOCKET clientSocket : m_clientSockets) {
        SendRandomNumberToClient(clientSocket);
    }

    // 게임 상태를 "게임 중"으로 변경
    gameState = 2;

    std::cout << "Game in progress..." << std::endl;
}

void CServer::EndGame() {
    if (gameState != 2) {
        std::cout << "No game in progress!" << std::endl;
        return;
    }

    // 게임 종료 상태로 변경
    gameState = 0;
    std::cout << "Game ended!" << std::endl;

    // 게임 종료 메시지 전송
    std::string gameEndMessage = "game_end";
    for (SOCKET clientSocket : m_clientSockets) {
        send(clientSocket, gameEndMessage.c_str(), gameEndMessage.length(), 0);
    }

    DisplayClientInfo();
}

int main() {
    CServer server;

    if (!server.Initialize()) {
        return -1;
    }

    server.StartListening();

    return 0;
}