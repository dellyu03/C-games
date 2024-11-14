// FileLister.cpp
#include "pch.h"
#include "FileLister.h"


FileLister::FileLister(const std::string& directoryPath) : directoryPath(directoryPath) {
    // 디렉토리 내 파일 목록을 가져옵니다.
    for (const auto& entry : fs::directory_iterator(directoryPath)) {
        if (fs::is_regular_file(entry)) {
            fileNames.push_back(entry.path().filename().string());  // 파일명만 저장
        }
    }
}

std::vector<std::string> FileLister::getFileNames() const {
    return fileNames;
}

std::string FileLister::getFileAt(int index) const {
    if (index >= 0 && index < fileNames.size()) {
        return fileNames[index];
    }
    return "";  // 유효하지 않은 인덱스인 경우 빈 문자열 반환
}
