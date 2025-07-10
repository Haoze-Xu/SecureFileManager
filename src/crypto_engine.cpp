#include "../include/crypto_engine.h"
#include <fstream>
#include <stdexcept>
#include <cmath>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <cryptopp/filters.h>
#include <cryptopp/files.h>
#include <cryptopp/osrng.h>
#include <cryptopp/pwdbased.h>
#include <cryptopp/sha.h>
#include <cryptopp/modes.h>
#include <cryptopp/aes.h>

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
                              const std::string& password,
                              ProgressCallback callback) {
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

        // 写入文件头 (16字节salt + 16字节IV)
        outFile.write(reinterpret_cast<const char*>(salt), sizeof(salt));
        outFile.write(reinterpret_cast<const char*>(iv), sizeof(iv));
        
        // 设置加密器 - 使用PKCS填充
        CryptoPP::CBC_Mode<CryptoPP::AES>::Encryption encryptor;
        encryptor.SetKeyWithIV(key, sizeof(key), iv);
        
        // 创建加密过滤器链
        CryptoPP::StreamTransformationFilter stfEncryptor(
            encryptor,
            new CryptoPP::FileSink(outFile),
            CryptoPP::BlockPaddingSchemeDef::PKCS_PADDING
        );
        
        // 分块处理文件
        const size_t bufferSize = 1 * 1024 * 1024; // 1MB
        std::vector<char> buffer(bufferSize);
        size_t totalBytes = 0;
        int lastProgress = -1; // 跟踪上一次的进度值
        
        while (inFile.read(buffer.data(), bufferSize)) {
            size_t bytesRead = static_cast<size_t>(inFile.gcount());
            stfEncryptor.Put(
                reinterpret_cast<const CryptoPP::byte*>(buffer.data()), 
                bytesRead
            );
            
            totalBytes += bytesRead;
            if (callback) {
                int newProgress = static_cast<int>((totalBytes * 100) / fileSize);
                // 只有当进度变化时才调用回调
                if (newProgress != lastProgress) {
                    callback(newProgress);
                    lastProgress = newProgress;
                }
            }
        }
        
        // 处理最后一块数据并添加填充
        size_t lastBytes = static_cast<size_t>(inFile.gcount());
        if (lastBytes > 0) {
            stfEncryptor.Put(
                reinterpret_cast<const CryptoPP::byte*>(buffer.data()), 
                lastBytes
            );
            totalBytes += lastBytes;
        }
        
        // 完成加密并写入填充
        stfEncryptor.MessageEnd();
        
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
                              const std::string& password,
                              ProgressCallback callback) {
    try {
        // 检查输入文件
        if (!fs::exists(inputPath)) {
            throw std::runtime_error("输入文件不存在: " + inputPath);
        }
        
        // 获取文件大小
        size_t fileSize = fs::file_size(inputPath);
        if (fileSize <= 32) { // 文件头大小 (16字节salt + 16字节IV)
            throw std::runtime_error("加密文件无效: " + inputPath);
        }
        
        // 打开输入文件
        std::ifstream inFile(inputPath, std::ios::binary);
        if (!inFile) {
            throw std::runtime_error("无法打开输入文件: " + inputPath);
        }
        
        // 读取文件头（盐和IV）
        char salt[16], iv[CryptoPP::AES::BLOCKSIZE];
        if (!inFile.read(salt, sizeof(salt)) || 
            !inFile.read(iv, sizeof(iv))) {
            throw std::runtime_error("无法读取加密文件头");
        }
        
        // 派生密钥
        CryptoPP::byte key[CryptoPP::AES::DEFAULT_KEYLENGTH];
        CryptoPP::PKCS5_PBKDF2_HMAC<CryptoPP::SHA256> pbkdf;
        pbkdf.DeriveKey(key, sizeof(key), 0, 
                       reinterpret_cast<const CryptoPP::byte*>(password.data()), 
                       password.size(),
                       reinterpret_cast<const CryptoPP::byte*>(salt), sizeof(salt), 10000);
        
        // 设置解密器 - 使用PKCS填充
        CryptoPP::CBC_Mode<CryptoPP::AES>::Decryption decryptor;
        decryptor.SetKeyWithIV(key, sizeof(key), reinterpret_cast<const CryptoPP::byte*>(iv));
        
        // 打开输出文件
        std::ofstream outFile(outputPath, std::ios::binary);
        if (!outFile) {
            throw std::runtime_error("无法创建输出文件: " + outputPath);
        }
        
        // 创建解密过滤器链
        CryptoPP::StreamTransformationFilter stfDecryptor(
            decryptor,
            new CryptoPP::FileSink(outFile),
            CryptoPP::BlockPaddingSchemeDef::PKCS_PADDING
        );
        
        // 分块解密（跳过32字节文件头）
        const size_t bufferSize = 1 * 1024 * 1024; // 1MB
        std::vector<char> buffer(bufferSize);
        size_t totalBytes = 0;
        size_t encryptedSize = fileSize - sizeof(salt) - sizeof(iv);
        int lastProgress = -1; // 跟踪上一次的进度值
        
        while (inFile.read(buffer.data(), bufferSize)) {
            size_t bytesRead = static_cast<size_t>(inFile.gcount());
            stfDecryptor.Put(
                reinterpret_cast<const CryptoPP::byte*>(buffer.data()), 
                bytesRead
            );
            
            totalBytes += bytesRead;
            if (callback) {
                int newProgress = static_cast<int>((totalBytes * 100) / encryptedSize);
                // 只有当进度变化时才调用回调
                if (newProgress != lastProgress) {
                    callback(newProgress);
                    lastProgress = newProgress;
                }
            }
        }
        
        // 处理最后一块数据
        size_t lastBytes = static_cast<size_t>(inFile.gcount());
        if (lastBytes > 0) {
            stfDecryptor.Put(
                reinterpret_cast<const CryptoPP::byte*>(buffer.data()), 
                lastBytes
            );
        }
        
        // 完成解密并移除填充
        stfDecryptor.MessageEnd();
        
        // 清理敏感数据
        secureWipe(key, sizeof(key));
        
        return true;
    } 
    catch (const CryptoPP::Exception& e) {
        // 精确的错误处理
        std::string error = e.what();
        if (error.find("InvalidCiphertext") != std::string::npos ||
            error.find("StreamTransformationFilter") != std::string::npos) {
            throw std::runtime_error("解密失败: 密码错误或文件已损坏");
        }
        throw std::runtime_error("解密错误: " + error);
    }
    catch (const std::exception& e) {
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