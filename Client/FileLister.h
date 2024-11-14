// FileLister.h
#ifndef FILE_LISTER_H
#define FILE_LISTER_H

#include <string>
#include <vector>

namespace fs = std::filesystem;

class FileLister {
public:
    // 생성자: 디렉토리 경로를 받아옵니다.
    FileLister(const std::string& directoryPath);

    // 디렉토리 내 파일 목록을 반환하는 메서드
    std::vector<std::string> getFileNames() const;

private:
    std::string directoryPath;  // 디렉토리 경로 저장
};

#endif // FILE_LISTER_H
