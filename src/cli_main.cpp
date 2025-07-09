#include "../include/crypto_engine.h"
#include "../include/file_processor.h"
#include <iostream>
#include <string>
#include <iomanip>
#include <filesystem>
#include <windows.h>

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    // 设置控制台为UTF-8编码
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    
    if (argc != 5) {
        std::cerr << "文件安全管理系统 - 命令行工具\n"
                  << "用法: " << argv[0] << " <模式> <输入文件> <输出文件> <密码>\n"
                  << "模式: -e 加密, -d 解密\n"
                  << "示例: " << argv[0] << " -e document.txt document.enc \"MyStrongP@ss\"\n"
                  << "当前工作目录: " << fs::current_path().string() << "\n";
        return 1;
    }
    
    std::string mode = argv[1];
    std::string inputPath = argv[2];
    std::string outputPath = argv[3];
    std::string password = argv[4];
    
    if (!FileProcessor::fileExists(inputPath)) {
        std::cerr << "错误: 输入文件不存在 - " << inputPath << "\n";
        return 2;
    }
    
    try {
        bool success = false;
        
        if (mode == "-e") {
            std::cout << "开始加密文件: " << inputPath << "\n";
            success = CryptoEngine::encryptFile(inputPath, outputPath, password);
            if (success) std::cout << "加密成功! 输出文件: " << outputPath << "\n";
        } 
        else if (mode == "-d") {
            std::cout << "开始解密文件: " << inputPath << "\n";
            success = CryptoEngine::decryptFile(inputPath, outputPath, password);
            if (success) std::cout << "解密成功! 输出文件: " << outputPath << "\n";
        }
        else {
            std::cerr << "错误: 无效模式 '" << mode << "'. 使用 -e 或 -d\n";
            return 3;
        }
        
        return success ? 0 : 4;
    } catch (const std::exception& e) {
        std::cerr << "操作失败: " << e.what() << "\n";
        return 5;
    }
}