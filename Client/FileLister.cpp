// FileLister.cpp
#include "pch.h"
#include <filesystem>
#include "FileLister.h"
#include <iostream>

namespace fs = std::filesystem;

// 생성자: 디렉토리 경로를 초기화
FileLister::FileLister(const std::string& directoryPath) : directoryPath(directoryPath) {}

// 디렉토리 내 파일 목록을 반환하는 메서드
std::vector<std::string> FileLister::getFileNames() const {
    std::vector<std::string> fileNames;

    // 지정된 경로가 디렉토리인지 확인
    if (fs::is_directory(directoryPath)) {
        // 디렉토리 내의 파일들을 반복
        for (const auto& entry : fs::directory_iterator(directoryPath)) {
            // 파일일 경우, 파일 이름을 리스트에 추가
            if (fs::is_regular_file(entry)) {
                fileNames.push_back(entry.path().filename().string());
            }
        }
    }
    else {
        std::cerr << "지정된 경로는 디렉토리가 아닙니다!" << std::endl;
    }

    return fileNames;
}
