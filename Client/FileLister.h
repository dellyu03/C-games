#include "pch.h"
#ifndef FILE_LISTER_H
#define FILE_LISTER_H

#include <string>
#include <vector>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

class FileLister {
public:
    // ������: ���丮 ��θ� �޾ƿɴϴ�.
    FileLister(const std::string& directoryPath);

    // ���丮 �� ���� ����� ��ȯ�ϴ� �޼���
    std::vector<std::string> getFileNames() const;

    // ������ �ε����� �ش��ϴ� ������ ��ȯ
    std::string getFileAt(int index) const;

private:
    std::string directoryPath;  // ���丮 ��� ����
    std::vector<std::string> fileNames;  // ���� ��� ����
};

#endif // FILE_LISTER_H
