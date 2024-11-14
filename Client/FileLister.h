// FileLister.h
#ifndef FILE_LISTER_H
#define FILE_LISTER_H

#include <string>
#include <vector>

namespace fs = std::filesystem;

class FileLister {
public:
    // ������: ���丮 ��θ� �޾ƿɴϴ�.
    FileLister(const std::string& directoryPath);

    // ���丮 �� ���� ����� ��ȯ�ϴ� �޼���
    std::vector<std::string> getFileNames() const;

private:
    std::string directoryPath;  // ���丮 ��� ����
};

#endif // FILE_LISTER_H
