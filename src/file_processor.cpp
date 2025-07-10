#include "../include/file_processor.h"
#include <filesystem>
#include <fstream>
#include <random>
#include <vector>
#include <cryptopp/sha.h>
#include <cryptopp/hex.h>
#include <cryptopp/files.h>
#include <cryptopp/filters.h>

namespace fs = std::filesystem;

bool FileProcessor::fileExists(const std::string& path) {
    return fs::exists(path);
}

size_t FileProcessor::fileSize(const std::string& path) {
    try {
        return fs::file_size(path);
    } catch (...) {
        return 0;
    }
}

bool FileProcessor::secureDelete(const std::string& path) {
    if (!fileExists(path)) return false;
    
    size_t size = fileSize(path);
    if (size == 0) {
        return fs::remove(path);
    }
    
    try {
        // 打开文件进行读写
        std::fstream file(path, std::ios::binary | std::ios::in | std::ios::out);
        if (!file) return false;
        
        // 覆盖模式
        const unsigned char patterns[3] = {0xFF, 0x00, 0xAA};
        std::vector<char> buffer(size);
        
        for (int i = 0; i < 3; i++) {
            file.seekp(0);
            
            if (i == 2) {
                // 随机数据覆盖
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<int> dist(0, 255);
                for (size_t j = 0; j < size; j++) {
                    buffer[j] = static_cast<char>(dist(gen));
                }
            } else {
                // 固定模式覆盖
                std::fill(buffer.begin(), buffer.end(), patterns[i]);
            }
            
            file.write(buffer.data(), size);
            file.flush();
        }
        
        file.close();
        return fs::remove(path);
    } catch (...) {
        return false;
    }
}

std::string FileProcessor::calculateSHA256(const std::string& path) {
    CryptoPP::SHA256 hash;
    std::string digest;
    
    try {
        CryptoPP::FileSource file(path.c_str(), true, 
            new CryptoPP::HashFilter(hash,
                new CryptoPP::HexEncoder(
                    new CryptoPP::StringSink(digest)
                )
            ));
        
        // 移除尾部换行符（如果存在）
        if (!digest.empty() && digest.back() == '\n') {
            digest.pop_back();
        }
    } catch (const std::exception& e) {
        throw std::runtime_error("计算哈希失败: " + std::string(e.what()));
    } catch (...) {
        throw std::runtime_error("未知错误导致哈希计算失败");
    }
    
    return digest;
}