#include "pch.h"
#include <iostream>
#include <winsock2.h>
#include <string>
#include <Ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_PORT 9000
#define BUF_SIZE 512

class CClient {
public:
    CClient();
    ~CClient();
    bool Initialize();
    bool ConnectToServer(const std::string& serverIp);  // 반환 타입을 bool로 변경
    void SendMessage(const std::string& message);
    void ReceiveMessage();

private:
    SOCKET m_socket;
    sockaddr_in m_serverAddr;
};

CClient::CClient() : m_socket(INVALID_SOCKET) {}

CClient::~CClient() {
    if (m_socket != INVALID_SOCKET) {
        closesocket(m_socket);
    }
    WSACleanup();
}

bool CClient::Initialize() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed!" << std::endl;
        return false;
    }
    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_socket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed!" << std::endl;
        return false;
    }
    return true;
}

bool CClient::ConnectToServer(const std::string& serverIp) {
    m_serverAddr.sin_family = AF_INET;
    m_serverAddr.sin_port = htons(SERVER_PORT);

    // inet_pton으로 IP 주소 변환
    if (inet_pton(AF_INET, serverIp.c_str(), &m_serverAddr.sin_addr) <= 0) {
        std::cerr << "Invalid address!" << std::endl;
        return false;
    }

    if (connect(m_socket, (SOCKADDR*)&m_serverAddr, sizeof(m_serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Connection failed!" << std::endl;
        return false;
    }
    std::cout << "Connected to server!" << std::endl;
    return true;
}

void CClient::SendMessage(const std::string& message) {
    send(m_socket, message.c_str(), message.length(), 0);
}

void CClient::ReceiveMessage() {
    char buffer[BUF_SIZE];
    int bytesReceived = recv(m_socket, buffer, BUF_SIZE, 0);
    if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0';
        std::cout << "Message from server: " << buffer << std::endl;
    }
    else {
        std::cerr << "No data received!" << std::endl;
    }
}

int main() {
    CClient client;
    if (client.Initialize()) {
        std::string serverIp;
        bool connected = false;

        // 서버 연결을 성공할 때까지 반복
        while (!connected) {
            std::cout << "Enter server IP: ";
            std::cin >> serverIp;  // 클라이언트가 서버 IP를 입력할 수 있게 함

            connected = client.ConnectToServer(serverIp);  // 서버에 연결

            if (!connected) {
                std::cerr << "Failed to connect to the server. Please try again." << std::endl;
            }
        }

        // 연결된 후 명령어 입력받기
        std::string command;
        while (true) {
            std::cout << "Enter command to send to server (or 'exit' to quit): ";
            std::cin >> command;  // 명령어 입력받기

            if (command == "exit") {
                break;  // 'exit' 입력 시 종료
            }

            client.SendMessage(command);  // 서버에 명령어 보내기
            client.ReceiveMessage();  // 서버로부터 응답 받기
        }

        std::cout << "Exiting..." << std::endl;
    }
    return 0;
}
