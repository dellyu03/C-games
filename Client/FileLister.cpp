// FileLister.cpp
#include "pch.h"
#include <filesystem>
#include "FileLister.h"
#include <iostream>

namespace fs = std::filesystem;

// ������: ���丮 ��θ� �ʱ�ȭ
FileLister::FileLister(const std::string& directoryPath) : directoryPath(directoryPath) {}

// ���丮 �� ���� ����� ��ȯ�ϴ� �޼���
std::vector<std::string> FileLister::getFileNames() const {
    std::vector<std::string> fileNames;

    // ������ ��ΰ� ���丮���� Ȯ��
    if (fs::is_directory(directoryPath)) {
        // ���丮 ���� ���ϵ��� �ݺ�
        for (const auto& entry : fs::directory_iterator(directoryPath)) {
            // ������ ���, ���� �̸��� ����Ʈ�� �߰�
            if (fs::is_regular_file(entry)) {
                fileNames.push_back(entry.path().filename().string());
            }
        }
    }
    else {
        std::cerr << "������ ��δ� ���丮�� �ƴմϴ�!" << std::endl;
    }

    return fileNames;
}
