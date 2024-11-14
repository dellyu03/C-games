#include "pch.h"
#include <winsock2.h>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <Ws2tcpip.h>
#include <filesystem>

#pragma comment(lib, "ws2_32.lib")

//서버 포트 및 버퍼 사이트
#define SERVER_PORT 9000
#define BUF_SIZE 512

//파일 시스템 객체 정의
namespace fs = std::filesystem;


//클라이언트 구조체
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
    std::vector<std::string> GetFileList(const std::string& directoryPath);

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

//클라이언트 생성시
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


//서버 커넥션 관련 명령어
void CClient::ConnectToServer(const std::string& serverIp) {
    m_serverAddr.sin_family = AF_INET;
    m_serverAddr.sin_port = htons(SERVER_PORT);

    //아이피 주소 틀렸을 때
    if (inet_pton(AF_INET, serverIp.c_str(), &m_serverAddr.sin_addr) <= 0) {
        std::cerr << "Invalid IP address!" << std::endl;
        return;
    }

    //주소는 맞지만 에러 코드가 났을 때
    int result = connect(m_socket, (SOCKADDR*)&m_serverAddr, sizeof(m_serverAddr));
    if (result == SOCKET_ERROR) {
        int errorCode = WSAGetLastError();
        std::cerr << "Connection failed with error code: " << errorCode << std::endl;
        return;
    }

    std::cout << "Connected to server!" << std::endl;
}

//Message send
void CClient::SendMessage(const std::string& message) {
    send(m_socket, message.c_str(), message.length(), 0);
}

//서버 메시지 리스폰스
void CClient::ReceiveMessage() {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(m_socket, &readfds);

    //서버 응답이 10초 없을 시 연결 해제
    timeval timeout;
    timeout.tv_sec = 10;  // 현재 10초 바꿔도 됩니다.
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

            // 서버에서 받은 메시지를 숫자 리스트로 파싱
            std::vector<int> numberList;
            std::istringstream iss(buffer);
            std::string token;
            while (iss >> token) {
                if (isdigit(token[0])) {  // 숫자인 경우만 추가
                    numberList.push_back(std::stoi(token));
                }
            }

            // 받은 숫자 리스트 출력
            std::cout << "Received Numbers: ";
            for (int num : numberList) {
                std::cout << num << " ";
            }
            std::cout << std::endl;

            // 파일 목록 가져오기
            std::string directoryPath = "C:\\Users\\ms\\Desktop\\dellyu03\\도움\\project\\img";
            std::vector<std::string> fileNames = GetFileList(directoryPath);

            // 받은 숫자에 해당하는 인덱스의 파일만 출력
            std::cout << "Selected Files: ";
            for (int num : numberList) {
                if (num - 1 < fileNames.size()) {  // 인덱스 조정: 1부터 시작하므로 -1
                    std::cout << fileNames[num - 1] << " ";
                }
            }
            std::cout << std::endl;
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

std::vector<std::string> CClient::GetFileList(const std::string& directoryPath) {
    std::vector<std::string> fileNames;

    try {
        for (const auto& entry : fs::directory_iterator(directoryPath)) {
            if (fs::is_regular_file(entry.status())) {
                fileNames.push_back(entry.path().filename().string());
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error reading directory: " << e.what() << std::endl;
    }

    return fileNames;
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

        bool validIp = false;
        while (!validIp) {
            // 서버 IP 유효성 검사
            struct sockaddr_in sa;
            int result = inet_pton(AF_INET, serverIp.c_str(), &(sa.sin_addr));
            if (result != 1) {  // 유효하지 않은 IP
                std::cerr << "Invalid IP address. Please try again." << std::endl;
                std::cout << "Enter server IP: ";
                std::cin >> serverIp;
            }
            else {
                validIp = true;  // 유효한 IP일 경우 루프 종료
            }
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
