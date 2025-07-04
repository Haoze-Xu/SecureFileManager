# ==================== 基础配置 ====================
TEMPLATE = app
TARGET = SecureFileManager
CONFIG += console c++17
CONFIG -= app_bundle qt gui widgets

# 完全禁用GUI相关模块
QT -= core gui widgets

# ==================== 源文件配置 ====================
SOURCES += src/cli_main.cpp \
           src/crypto_engine.cpp \
           src/file_processor.cpp

HEADERS += include/crypto_engine.h \
           include/file_processor.h

# ==================== Crypto++ 配置 ====================
# 头文件路径
INCLUDEPATH += "D:/_SecureFileManager/cryptopp/include"
# 库文件路径
LIBS += -L"D:/_SecureFileManager/cryptopp/lib" -lcryptopp

# ==================== Windows 特定配置 ====================
win32 {
    # 静态链接
    CONFIG += static
    QMAKE_CXXFLAGS += -static
    QMAKE_LFLAGS += -static
    
    # 控制台子系统
    QMAKE_LFLAGS += -Wl,-subsystem,console
    
    # 禁用GUI相关处理
    QMAKE_UIC = 
    QMAKE_MOC = 
    
    # 添加文件系统库支持
    LIBS += -lstdc++fs
}

# ==================== 编译器标志 ====================
# 启用所有警告
QMAKE_CXXFLAGS += -Wall -Wextra -pedantic
# 优化级别
CONFIG(release, debug|release) {
    QMAKE_CXXFLAGS_RELEASE = -O2
}
CONFIG(debug, debug|release) {
    QMAKE_CXXFLAGS_DEBUG = -g
}