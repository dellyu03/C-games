#include "pch.h"
#include <winsock2.h>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <Ws2tcpip.h>
#include <filesystem>
#include <thread>  // 멀티스레딩 라이브러리 추가
#include <atomic>
#include <chrono>
#include <mutex>

#pragma comment(lib, "ws2_32.lib")

// 서버 포트 및 버퍼 사이즈
#define SERVER_PORT 9000
#define BUF_SIZE 512

// 파일 시스템 객체 정의
namespace fs = std::filesystem;

// 클라이언트 구조체
class CClient {
public:
    CClient();
    ~CClient();
    bool Initialize();
    void ConnectToServer(const std::string& serverIp);
    void SendMessage(const std::string& message);
    void ReceiveMessagesAsync();
    void UserInput();
    void CheckConnection();
    void Disconnect();
    bool IsConnected();
    std::vector<std::string> GetFileList(const std::string& directoryPath);
    void HandleCompletedMessage();

private:
    SOCKET m_socket;
    sockaddr_in m_serverAddr;
    std::atomic<bool> m_isConnected{ false };  // 연결 상태를 관리하기 위해 atomic 사용
    std::atomic<bool> m_isWaitingForCommand{ false };  // 명령 대기 상태 관리
    std::mutex m_inputMutex;  // 입력과 출력을 동기화할 뮤텍스
    std::vector<int> m_numberList;  // 서버에서 받은 숫자 리스트
};

CClient::CClient() : m_socket(INVALID_SOCKET) {}

CClient::~CClient() {
    if (m_socket != INVALID_SOCKET) {
        closesocket(m_socket);
    }
    WSACleanup();
}

// 클라이언트 생성시
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

// 서버 커넥션 관련 명령어
void CClient::ConnectToServer(const std::string& serverIp) {
    m_serverAddr.sin_family = AF_INET;
    m_serverAddr.sin_port = htons(SERVER_PORT);

    // 아이피 주소 틀렸을 때
    if (inet_pton(AF_INET, serverIp.c_str(), &m_serverAddr.sin_addr) <= 0) {
        std::cerr << "Invalid IP address!" << std::endl;
        return;
    }

    // 주소는 맞지만 에러 코드가 났을 때
    int result = connect(m_socket, (SOCKADDR*)&m_serverAddr, sizeof(m_serverAddr));
    if (result == SOCKET_ERROR) {
        int errorCode = WSAGetLastError();
        std::cerr << "Connection failed with error code: " << errorCode << std::endl;
        return;
    }

    m_isConnected = true;
    std::cout << "Connected to server!" << std::endl;
}

// 메시지 전송
void CClient::SendMessage(const std::string& message) {
    send(m_socket, message.c_str(), message.length(), 0);
}

void CClient::ReceiveMessagesAsync() {
    while (m_isConnected) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(m_socket, &readfds);

        timeval timeout;
        timeout.tv_sec = 0;  // 0초 대기 (빠른 메시지 수신)
        timeout.tv_usec = 10000;  // 10ms 대기

        int selectResult = select(0, &readfds, nullptr, nullptr, &timeout);

        if (selectResult > 0) {
            char buffer[BUF_SIZE];
            int bytesReceived = recv(m_socket, buffer, BUF_SIZE, 0);

            if (bytesReceived == SOCKET_ERROR) {
                std::cerr << "Server connection lost!" << std::endl;
                Disconnect();
                break;
            }
            else if (bytesReceived == 0) {
                std::cerr << "Server closed the connection!" << std::endl;
                Disconnect();
                break;
            }
            else {
                buffer[bytesReceived] = '\0';
                std::string receivedMessage(buffer);

                // "completed" 메시지를 받으면 명령 대기 상태로 돌아감
                if (receivedMessage == "completed") {
                    HandleCompletedMessage();
                    continue;
                }

                std::cout << receivedMessage << std::endl;

                // "number_list:" 이후의 숫자들만 파싱
                std::vector<int> numberList;
                std::istringstream iss(receivedMessage);
                std::string token;
                bool isNumberList = false;  // number_list: 이후 숫자만 파싱할 플래그

                // "number_list:" 이후의 숫자들만 추출
                while (iss >> token) {
                    if (token == "number_list:") {
                        isNumberList = true;  // number_list: 이후로부터 숫자만 추출
                        continue;  // "number_list:" 부분을 건너뜀
                    }

                    // number_list: 이후에만 숫자를 추가
                    if (isNumberList && isdigit(token[0])) {
                        numberList.push_back(std::stoi(token));
                    }
                }

                // 받은 숫자에 해당하는 인덱스의 파일만 출력
                if (!numberList.empty()) {
                    std::string directoryPath = "C:\\Users\\ms\\Desktop\\dellyu03\\project\\img";   // 원하는 파일 경로
                    std::vector<std::string> fileNames = GetFileList(directoryPath);

                    // 받은 숫자에 해당하는 인덱스의 파일만 출력
                    std::cout << "Selected Files: ";
                    for (int num : numberList) {
                        if (num - 1 < fileNames.size()) {  // 인덱스는 1부터 시작하므로 -1로 맞추기
                            std::cout << fileNames[num - 1] << " ";
                        }
                    }
                    std::cout << std::endl;
                }
            }
        }
        else if (selectResult == 0) {  // 타임아웃 발생
            // 타임아웃 대기 시간 동안 다른 작업 수행할 수 있음
        }
        else {  // select 오류
            std::cerr << "Error in receiving data from server!" << std::endl;
            Disconnect();
            break;
        }
    }
}



void CClient::UserInput() {
    while (true) {
        std::string command;

        // 연결 상태에 따라 명령어 입력 대기
        if (m_isConnected) {
            std::cout << "Enter command to send to server: ";
        }
        else {
            std::cout << "Not connected to server. Enter 'connect' to reconnect or 'exit' to quit: ";
        }

        std::cin >> command;

        if (command == "exit") {
            m_isConnected = false;
            break;
        }
        else if (command == "connect") {
            std::string serverIp;
            std::cout << "Enter server IP to reconnect: ";
            std::cin >> serverIp;
            ConnectToServer(serverIp);
        }
        else if (m_isConnected) {
            SendMessage(command);  // 서버에 입력된 명령어 전송
        }
        else {
            std::cout << "Cannot send command. Please connect to the server first." << std::endl;
        }
    }
}

void CClient::CheckConnection() {
    while (m_isConnected) {
        if (!IsConnected()) {
            std::cout << "Connection lost. Do you want to reconnect? (y/n): ";
            std::string response;
            std::cin >> response;

            if (response == "y" || response == "Y") {
                std::string serverIp;
                std::cout << "Enter server IP to reconnect: ";
                std::cin >> serverIp;
                ConnectToServer(serverIp);
            }
            else {
                std::cout << "Exiting client..." << std::endl;
                m_isConnected = false;
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(3));  // 3초마다 연결 상태 점검
    }
}

void CClient::Disconnect() {
    if (m_socket != INVALID_SOCKET) {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        m_isConnected = false;
        std::cout << "Disconnected from server." << std::endl;
    }
}

bool CClient::IsConnected() {
    return m_socket != INVALID_SOCKET;
}

std::vector<std::string> CClient::GetFileList(const std::string& directoryPath) {
    std::vector<std::string> fileNames;

    for (const auto& entry : fs::directory_iterator(directoryPath)) {
        if (fs::is_regular_file(entry.status())) {
            fileNames.push_back(entry.path().filename().string());
        }
    }

    return fileNames;
}

void CClient::HandleCompletedMessage() {
    // 명령 대기 상태로 돌아가기
    std::cout << "Received 'completed' message. Returning to waiting state." << std::endl;
    m_isWaitingForCommand = true;  // 대기 상태로 설정
}

int main() {
    CClient client;
    if (client.Initialize()) {
        // 프로그램 시작 시 자동으로 로컬 서버에 연결
        std::string serverIp = "127.0.0.1";  // 로컬 서버 IP로 고정
        client.ConnectToServer(serverIp);

        // 서버에서 받은 메시지를 비동기적으로 처리하는 스레드
        std::thread receiveThread(&CClient::ReceiveMessagesAsync, &client);
        // 사용자 입력을 받는 스레드
        std::thread inputThread(&CClient::UserInput, &client);

        // 클라이언트가 종료될 때까지 대기
        inputThread.join();
        receiveThread.join();
    }

    return 0;
}

