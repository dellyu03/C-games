#include "pch.h"
#include <winsock2.h>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <Ws2tcpip.h>
#include <filesystem>
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>
#include <set>
#include <random> // std::random_device, std::mt19937

#pragma comment(lib, "ws2_32.lib")

#define SERVER_PORT 9000
#define BUF_SIZE 512

namespace fs = std::filesystem;

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
    void StartGame(); // 게임 시작
    void EndGame();   // 게임 종료

private:
    SOCKET m_socket;
    sockaddr_in m_serverAddr;
    std::atomic<bool> m_isConnected{ false };
    std::atomic<bool> m_isWaitingForCommand{ false };
    std::mutex m_inputMutex;
    std::vector<int> m_numberList;
    bool m_isGameInProgress{ false }; // 게임 진행 중 상태
    std::vector<std::string> m_selectedFiles; // 선택된 파일들
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
        std::cerr << "Invalid IP address!" << std::endl;
        return;
    }

    int result = connect(m_socket, (SOCKADDR*)&m_serverAddr, sizeof(m_serverAddr));
    if (result == SOCKET_ERROR) {
        int errorCode = WSAGetLastError();
        std::cerr << "Connection failed with error code: " << errorCode << std::endl;
        return;
    }

    m_isConnected = true;
    std::cout << "Connected to server!" << std::endl;
}

void CClient::SendMessage(const std::string& message) {
    send(m_socket, message.c_str(), message.length(), 0);
}

void CClient::ReceiveMessagesAsync() {
    while (m_isConnected) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(m_socket, &readfds);

        timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 10000;

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

                if (receivedMessage == "completed") {
                    HandleCompletedMessage();
                    continue;
                }

                std::cout << receivedMessage << std::endl;

                std::vector<int> numberList;
                std::istringstream iss(receivedMessage);
                std::string token;
                bool isNumberList = false;

                while (iss >> token) {
                    if (token == "number_list:") {
                        isNumberList = true;
                        continue;
                    }
                    if (isNumberList && isdigit(token[0])) {
                        numberList.push_back(std::stoi(token));
                    }
                }

                if (!numberList.empty()) {
                    std::string directoryPath = "C:\\Users\\ms\\Desktop\\dellyu03\\project\\img";
                    std::vector<std::string> fileNames = GetFileList(directoryPath);

                    m_selectedFiles.clear();  // 이전 선택 파일 목록 초기화
                    for (int num : numberList) {
                        if (num - 1 < fileNames.size()) {
                            m_selectedFiles.push_back(fileNames[num - 1]);
                            std::string pairedFile = fileNames[num - 1];
                            size_t dotPos = pairedFile.rfind(".");
                            if (dotPos != std::string::npos) {
                                pairedFile.insert(dotPos, "짝");
                                m_selectedFiles.push_back(pairedFile);
                            }
                        }
                    }

                    std::random_device rd;
                    std::mt19937 g(rd());
                    std::shuffle(m_selectedFiles.begin(), m_selectedFiles.end(), g);

                    std::cout << "Selected Files:\n";
                    for (size_t i = 0; i < m_selectedFiles.size(); ++i) {
                        std::cout << i + 1 << ") " << m_selectedFiles[i] << "\n";
                    }

                    StartGame(); // 게임 시작
                }
            }
        }
        else if (selectResult == 0) {
        }
        else {
            std::cerr << "Error in receiving data from server!" << std::endl;
            Disconnect();
            break;
        }
    }
}

void CClient::UserInput() {
    while (true) {
        std::string command;

        if (m_isConnected && !m_isGameInProgress) {
            // 게임이 진행 중이지 않으면 명령 입력 받기
            std::cout << "Enter command to send to server: ";
            std::cin >> command;
            std::cin.ignore();  // 버퍼에 남은 엔터 입력을 처리

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
                SendMessage(command);
            }
            else {
                std::cout << "Cannot send command. Please connect to the server first." << std::endl;
            }
        }
        else if (m_isGameInProgress) {
            // 게임 진행 중일 때 정답 입력 받기
            std::cout << "\nEnter two numbers to check files (e.g., '1 3'): ";
            std::string input;
            std::getline(std::cin, input);  // 사용자 입력 받기

            std::istringstream inputStream(input);
            int number1, number2;
            if (inputStream >> number1 >> number2) {
                if (number1 > 0 && number1 <= m_selectedFiles.size() &&
                    number2 > 0 && number2 <= m_selectedFiles.size()) {

                    std::string file1 = m_selectedFiles[number1 - 1];
                    std::string file2 = m_selectedFiles[number2 - 1];

                    // 파일 짝 맞추기 검사
                    if (file1.find("짝") != std::string::npos &&
                        file2 == file1.substr(0, file1.find("짝")) + file1.substr(file1.find("짝") + 2)) {
                        std::cout << "Correct! You've found a match: " << file1 << " and " << file2 << std::endl;
                        SendMessage("correct");
                        break;
                    }
                    else if (file2.find("짝") != std::string::npos &&
                        file1 == file2.substr(0, file2.find("짝")) + file2.substr(file2.find("짝") + 2)) {
                        std::cout << "Correct! You've found a match: " << file1 << " and " << file2 << std::endl;
                        SendMessage("correct");
                        break;
                    }
                    else {
                        std::cout << "Wrong! These files do not match. Try again!" << std::endl;
                        SendMessage("wrong");
                    }
                }
                else {
                    std::cout << "Invalid input. Please enter two valid file numbers separated by a space." << std::endl;
                }
            }
            else {
                std::cout << "Unknown command. Please enter two numbers separated by a space." << std::endl;
            }
        }
    }
}

void CClient::Disconnect() {
    m_isConnected = false;
    std::cout << "Disconnected from the server." << std::endl;
    closesocket(m_socket);
    WSACleanup();
}

void CClient::HandleCompletedMessage() {
    std::cout << "All clients completed the task." << std::endl;
    EndGame();
}

void CClient::StartGame() {
    m_isGameInProgress = true;
    std::cout << "Game started!" << std::endl;
}

void CClient::EndGame() {
    m_isGameInProgress = false;
    std::cout << "Game over!" << std::endl;
}

std::vector<std::string> CClient::GetFileList(const std::string& directoryPath) {
    std::vector<std::string> fileNames;
    for (const auto& entry : fs::directory_iterator(directoryPath)) {
        if (entry.is_regular_file()) {
            fileNames.push_back(entry.path().filename().string());
        }
    }
    return fileNames;
}

int main() {
    CClient client;

    if (!client.Initialize()) {
        return -1;
    }

    client.ConnectToServer("127.0.0.1");

    std::thread receiveThread([&client]() {
        client.ReceiveMessagesAsync();
        });

    client.UserInput();

    receiveThread.join();

    return 0;
}
