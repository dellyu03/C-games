#pragma once

// pch.h를 반드시 포함해야 합니다.
#include "pch.h"  // pch.h가 프로젝트 내에 이미 있어야 합니다.

#include <winsock2.h>
#include <vector>
#include <string>

#define SERVERPORT 9000
#define BUFSIZE    512

class CServer {
public:
    CServer();
    ~CServer();
    bool Initialize();
    void StartListening();
    void SendRandomNumberToClients();
    void DisplayClientInfo();

private:
    SOCKET m_listenSocket;  // 서버용 소켓
    std::vector<SOCKET> m_clientSockets;  // 클라이언트 소켓들을 저장하는 벡터
};