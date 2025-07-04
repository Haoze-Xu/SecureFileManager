#ifndef FILE_PROCESSOR_H
#define FILE_PROCESSOR_H

#include <string>

class FileProcessor {
public:
    static bool fileExists(const std::string& path);
    static size_t fileSize(const std::string& path);
    static bool secureDelete(const std::string& path);
};

#endif // FILE_PROCESSOR_H