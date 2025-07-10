#include "../include/crypto_engine.h"
#include <fstream>
#include <stdexcept>
#include <cmath>
#include <cctype>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

// 密钥派生实现
void CryptoEngine::deriveKey(const std::string& password, 
                           CryptoPP::byte* key, size_t keySize, 
                           CryptoPP::byte* salt, size_t saltSize) {
    CryptoPP::AutoSeededRandomPool rng;
    rng.GenerateBlock(salt, saltSize);
    
    CryptoPP::PKCS5_PBKDF2_HMAC<CryptoPP::SHA256> pbkdf;
    pbkdf.DeriveKey(key, keySize, 0, 
                   reinterpret_cast<const CryptoPP::byte*>(password.data()), 
                   password.size(),
                   salt, saltSize, 10000);
}

// 文件加密实现
bool CryptoEngine::encryptFile(const std::string& inputPath, 
                              const std::string& outputPath, 
                              const std::string& password) {
    try {
        // 检查输入文件
        if (!fs::exists(inputPath)) {
            throw std::runtime_error("输入文件不存在: " + inputPath);
        }
        
        // 获取文件大小
        size_t fileSize = fs::file_size(inputPath);
        if (fileSize == 0) {
            throw std::runtime_error("输入文件为空: " + inputPath);
        }
        
        // 打开输入文件
        std::ifstream inFile(inputPath, std::ios::binary);
        if (!inFile) {
            throw std::runtime_error("无法打开输入文件: " + inputPath);
        }
        
        // 打开输出文件
        std::ofstream outFile(outputPath, std::ios::binary);
        if (!outFile) {
            throw std::runtime_error("无法创建输出文件: " + outputPath);
        }
        
        // 生成密钥材料
        CryptoPP::byte key[CryptoPP::AES::DEFAULT_KEYLENGTH];
        CryptoPP::byte salt[16];
        deriveKey(password, key, sizeof(key), salt, sizeof(salt));
        
        // 生成随机IV
        CryptoPP::byte iv[CryptoPP::AES::BLOCKSIZE];
        CryptoPP::AutoSeededRandomPool rng;
        rng.GenerateBlock(iv, sizeof(iv));
        
        // 写入文件头
        outFile.write(reinterpret_cast<const char*>(salt), sizeof(salt));
        outFile.write(reinterpret_cast<const char*>(iv), sizeof(iv));
        
        // 设置加密器
        CryptoPP::CBC_Mode<CryptoPP::AES>::Encryption encryptor;
        encryptor.SetKeyWithIV(key, sizeof(key), iv);
        
        // 分块处理
        const size_t bufferSize = 4096;
        char buffer[bufferSize];
        size_t totalBytes = 0;
        
        while (inFile.read(buffer, bufferSize)) {
            size_t bytesRead = inFile.gcount();
            std::string ciphertext;
            
            CryptoPP::StringSource ss(
                reinterpret_cast<const CryptoPP::byte*>(buffer), bytesRead, true,
                new CryptoPP::StreamTransformationFilter(
                    encryptor, new CryptoPP::StringSink(ciphertext)
                )
            );
            
            outFile.write(ciphertext.data(), ciphertext.size());
            totalBytes += bytesRead;  // 正确统计原始字节
        }
        
        // 处理最后一块
        if (inFile.eof() && inFile.gcount() > 0) {
            size_t bytesRead = inFile.gcount();
            std::string ciphertext;
            
            CryptoPP::StringSource ss(
                reinterpret_cast<const CryptoPP::byte*>(buffer), bytesRead, true,
                new CryptoPP::StreamTransformationFilter(
                    encryptor, new CryptoPP::StringSink(ciphertext)
                )
            );
            
            outFile.write(ciphertext.data(), ciphertext.size());
            totalBytes += bytesRead;
        }
        
        std::cout << "加密完成: " << totalBytes << " 字节处理" << std::endl;
        
        // 清理敏感数据
        secureWipe(key, sizeof(key));
        secureWipe(salt, sizeof(salt));
        secureWipe(iv, sizeof(iv));
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "加密错误: " << e.what() << std::endl;
        throw std::runtime_error(std::string("加密失败: ") + e.what());
    }
}

// 文件解密实现
bool CryptoEngine::decryptFile(const std::string& inputPath, 
                              const std::string& outputPath, 
                              const std::string& password) {
    try {
        // 检查输入文件
        if (!fs::exists(inputPath)) {
            throw std::runtime_error("输入文件不存在: " + inputPath);
        }
        
        // 获取文件大小
        size_t fileSize = fs::file_size(inputPath);
        if (fileSize <= 32) { // 文件头大小
            throw std::runtime_error("加密文件无效: " + inputPath);
        }
        
        // 打开输入文件
        std::ifstream inFile(inputPath, std::ios::binary);
        if (!inFile) {
            throw std::runtime_error("无法打开输入文件: " + inputPath);
        }
        
        // 读取文件头（盐和IV）
        char salt[16], iv[CryptoPP::AES::BLOCKSIZE];
        inFile.read(salt, sizeof(salt));
        inFile.read(iv, sizeof(iv));
        
        // 派生密钥
        CryptoPP::byte key[CryptoPP::AES::DEFAULT_KEYLENGTH];
        CryptoPP::PKCS5_PBKDF2_HMAC<CryptoPP::SHA256> pbkdf;
        pbkdf.DeriveKey(key, sizeof(key), 0, 
                       reinterpret_cast<const CryptoPP::byte*>(password.data()), 
                       password.size(),
                       reinterpret_cast<const CryptoPP::byte*>(salt), sizeof(salt), 10000);
        
        // 设置解密器
        CryptoPP::CBC_Mode<CryptoPP::AES>::Decryption decryptor;
        decryptor.SetKeyWithIV(key, sizeof(key), reinterpret_cast<const CryptoPP::byte*>(iv));
        
        // 打开输出文件
        std::ofstream outFile(outputPath, std::ios::binary);
        if (!outFile) {
            throw std::runtime_error("无法创建输出文件: " + outputPath);
        }
        
        // 分块解密
        const size_t bufferSize = 4096;
        char buffer[bufferSize];
        size_t totalBytes = 0;
        size_t decryptedBytes = 0;  // 新增解密字节统计
        
        while (inFile.read(buffer, bufferSize)) {
            size_t bytesRead = inFile.gcount();
            std::string plaintext;
            
            CryptoPP::StringSource ss(
                reinterpret_cast<const CryptoPP::byte*>(buffer), bytesRead, true,
                new CryptoPP::StreamTransformationFilter(
                    decryptor, new CryptoPP::StringSink(plaintext)
                )
            );
            
            outFile.write(plaintext.data(), plaintext.size());
            totalBytes += bytesRead;
            decryptedBytes += plaintext.size();  // 统计解密后的字节
        }
        
        // 处理最后一块
        if (inFile.eof() && inFile.gcount() > 0) {
            size_t bytesRead = inFile.gcount();
            std::string plaintext;
            
            CryptoPP::StringSource ss(
                reinterpret_cast<const CryptoPP::byte*>(buffer), bytesRead, true,
                new CryptoPP::StreamTransformationFilter(
                    decryptor, new CryptoPP::StringSink(plaintext)
                )
            );
            
            outFile.write(plaintext.data(), plaintext.size());
            totalBytes += bytesRead;
            decryptedBytes += plaintext.size();
        }
        
        // 修正显示：显示实际解密的字节数
        std::cout << "解密完成: " << decryptedBytes << " 字节处理" << std::endl;
        
        // 清理敏感数据
        secureWipe(key, sizeof(key));
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "解密错误: " << e.what() << std::endl;
        throw std::runtime_error(std::string("解密失败: ") + e.what());
    }
}

// 检查是否为加密文件
bool CryptoEngine::isEncryptedFile(const std::string& path) {
    try {
        if (!fs::exists(path)) return false;
        
        // 加密文件最小大小 = salt(16) + IV(16) + 最小加密块(16)
        size_t minEncSize = 16 + CryptoPP::AES::BLOCKSIZE + CryptoPP::AES::BLOCKSIZE;
        if (fs::file_size(path) < minEncSize) return false;
        
        std::ifstream file(path, std::ios::binary);
        char header[32]; // 读取文件头
        file.read(header, sizeof(header));
        
        // 简单验证：检查前16字节是否为有效的盐值
        // 实际应用中可能需要更复杂的验证
        return file.gcount() == sizeof(header);
    } catch (...) {
        return false;
    }
}

// 安全内存擦除
void CryptoEngine::secureWipe(void* ptr, size_t size) {
    volatile unsigned char* p = static_cast<volatile unsigned char*>(ptr);
    while (size--) *p++ = 0;
}

// 密码强度检测
int CryptoEngine::passwordStrength(const std::string& password) {
    if (password.empty()) return 0;
    
    bool hasUpper = false, hasLower = false, hasDigit = false, hasSpecial = false;
    int score = 0;
    
    for (char c : password) {
        if (std::isupper(c)) hasUpper = true;
        if (std::islower(c)) hasLower = true;
        if (std::isdigit(c)) hasDigit = true;
        if (std::ispunct(c)) hasSpecial = true;
    }
    
    // 长度得分
    if (password.length() >= 8) score += 2;
    if (password.length() >= 12) score += 2;
    
    // 复杂度得分
    if (hasUpper) score++;
    if (hasLower) score++;
    if (hasDigit) score++;
    if (hasSpecial) score++;
    
    return (score > 10) ? 10 : score;
}