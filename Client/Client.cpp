#include "pch.h"
#include <winsock2.h>
#include <iostream>
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
    void ConnectToServer(const std::string& serverIp);
    void SendMessage(const std::string& message);
    void ReceiveMessage();
    void Disconnect();
    bool IsConnected();

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

void CClient::ConnectToServer(const std::string& serverIp) {
    m_serverAddr.sin_family = AF_INET;
    m_serverAddr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, serverIp.c_str(), &m_serverAddr.sin_addr) <= 0) {
        std::cerr << "Invalid address!" << std::endl;
        return;
    }

    int result = connect(m_socket, (SOCKADDR*)&m_serverAddr, sizeof(m_serverAddr));
    if (result == SOCKET_ERROR) {
        int errorCode = WSAGetLastError();  // 오류 코드 가져오기
        std::cerr << "Connection failed with error code: " << errorCode << std::endl;

        // 오류 코드에 대한 설명을 출력
        switch (errorCode) {
        case WSAECONNREFUSED:
            std::cerr << "Connection refused. The server may not be running or the port is closed." << std::endl;
            break;
        case WSAETIMEDOUT:
            std::cerr << "Connection timed out." << std::endl;
            break;
        case WSAEADDRNOTAVAIL:
            std::cerr << "Invalid IP address or network unreachable." << std::endl;
            break;
        default:
            std::cerr << "Unknown error." << std::endl;
            break;
        }

        return;
    }

    std::cout << "Connected to server!" << std::endl;
}

void CClient::SendMessage(const std::string& message) {
    send(m_socket, message.c_str(), message.length(), 0);
}

void CClient::ReceiveMessage() {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(m_socket, &readfds);

    timeval timeout;
    timeout.tv_sec = 10;  // 10초 타임아웃 설정
    timeout.tv_usec = 0;

    int selectResult = select(0, &readfds, nullptr, nullptr, &timeout);

    if (selectResult > 0) {  // 서버에서 메시지를 수신할 준비가 된 경우
        char buffer[BUF_SIZE];
        int bytesReceived = recv(m_socket, buffer, BUF_SIZE, 0);

        if (bytesReceived == SOCKET_ERROR) {
            std::cerr << "Server connection lost!" << std::endl;
            Disconnect();
        }
        else if (bytesReceived == 0) {
            std::cerr << "Server closed the connection!" << std::endl;
            Disconnect();
        }
        else {
            buffer[bytesReceived] = '\0';
            std::cout << "Message from server: " << buffer << std::endl;
        }
    }
    else if (selectResult == 0) {  // 타임아웃 발생
        std::cerr << "Request timed out. Server response took too long." << std::endl;
    }
    else {  // select 오류
        std::cerr << "Error in receiving data from server!" << std::endl;
        Disconnect();
    }
}


void CClient::Disconnect() {
    if (m_socket != INVALID_SOCKET) {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        std::cout << "Disconnected from server." << std::endl;
    }
}

bool CClient::IsConnected() {
    return m_socket != INVALID_SOCKET;
}

int main() {
    CClient client;
    std::string serverIp;

    while (true) {
        std::cout << "Enter server IP (or type 'exit' to quit): ";
        std::cin >> serverIp;

        if (serverIp == "exit") {
            break;  // 프로그램 종료
        }

        if (client.Initialize()) {
            client.ConnectToServer(serverIp);

            if (client.IsConnected()) {
                std::string command;
                while (client.IsConnected()) {
                    std::cout << "Enter command to send to server: ";
                    std::cin >> command;

                    client.SendMessage(command);  // 서버에 입력된 명령어 전송
                    client.ReceiveMessage();      // 서버로부터 메시지 받기
                }
            }
            else {
                std::cerr << "Failed to connect to the server!" << std::endl;
            }
        }
        else {
            std::cerr << "Failed to initialize client!" << std::endl;
        }
    }

    return 0;
}
