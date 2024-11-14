// FileLister.cpp
#include "pch.h"
#include "FileLister.h"


FileLister::FileLister(const std::string& directoryPath) : directoryPath(directoryPath) {
    // ���丮 �� ���� ����� �����ɴϴ�.
    for (const auto& entry : fs::directory_iterator(directoryPath)) {
        if (fs::is_regular_file(entry)) {
            fileNames.push_back(entry.path().filename().string());  // ���ϸ� ����
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
    return "";  // ��ȿ���� ���� �ε����� ��� �� ���ڿ� ��ȯ
}
