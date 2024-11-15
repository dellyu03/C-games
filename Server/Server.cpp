#include "pch.h"
#include <winsock2.h>
#include <iostream>
#include <vector>
#include <ctime>
#include <string>
#include <thread>
#include <algorithm>  // std::remove 사용을 위한 헤더
#include <random>

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
    void ReceiveCommandsFromClient(SOCKET clientSocket);  // 명령어 수신 함수 변경

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


//서버 시작시 시작되는 코드
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

//서버 명령어 대기 관련 코드
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

//랜덤 숫자 생성 코드
void CServer::SendRandomNumberToClient(SOCKET clientSocket) {
    // 1부터 20까지의 숫자를 벡터에 저장
    // <= 오른쪽 숫자를 늘려서 범위 지정 가능
    std::vector<int> numbers;
    for (int i = 1; i <= 20; ++i) {
        numbers.push_back(i);
    }

    // 랜덤 셔플을 위한 랜덤 엔진과 분포
    std::random_device rd;
    std::mt19937 g(rd());  // Mersenne Twister 엔진을 사용
    std::shuffle(numbers.begin(), numbers.end(), g);  // 벡터 내 숫자 랜덤하게 섞기

    //뽑을 사진 개수 선택
    // 첫 16개 숫자를 랜덤으로 선택
    numbers.resize(16);

    // 결과 메시지 생성
    std::string message = "";
    for (int num : numbers) {
        message += std::to_string(num) + " ";
    }

    // 클라이언트에게 메시지 전송
    send(clientSocket, message.c_str(), message.length(), 0);
    std::cout << "Sent random numbers to client: " << message << std::endl;
}

void CServer::DisplayClientInfo() {
    std::cout << "Current number of clients: " << m_clientSockets.size() << std::endl;

    // 서버 콘솔에 각 클라이언트 소켓 ID 출력
    for (size_t i = 0; i < m_clientSockets.size(); ++i) {
        std::cout << "Client " << i + 1 << ": Socket ID: " << m_clientSockets[i] << std::endl;
    }

    // 클라이언트 수를 문자열로 변환하여 전송
    std::string clientCountMessage = "Current number of clients: " + std::to_string(m_clientSockets.size());
    
    // 접속된 클라이언트 수 정보를 요청한 클라이언트로 전송
    for (const auto& clientSocket : m_clientSockets) {
        send(clientSocket, clientCountMessage.c_str(), clientCountMessage.length(), 0);
    }

    std::cout << "Sent client count to requesting client: " << clientCountMessage << std::endl;
}


//클라이언트 명령어 리시브 코드
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
                SendRandomNumberToClient(clientSocket);
            }
            // 클라이언트로부터 명령어가 "list"이면 클라이언트 정보 출력
            else if (strcmp(buffer, "list") == 0) {
                DisplayClientInfo();
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
        }
        else {
            std::cerr << "Failed to receive data from client!" << std::endl;
            break; // 오류 발생 시 연결 종료
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
