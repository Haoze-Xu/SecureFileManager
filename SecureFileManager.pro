# ==================== 基础配置 ====================
TEMPLATE = app
TARGET = SecureFileManager
CONFIG += console c++17
CONFIG -= app_bundle

# 启用Qt GUI模块
QT += core gui widgets

# ==================== 源文件配置 ====================
SOURCES += src/crypto_engine.cpp \
           src/file_processor.cpp \
           src/mainwindow.cpp \
           src/main.cpp  # GUI主入口

HEADERS += include/crypto_engine.h \
           include/file_processor.h \
           include/mainwindow.h

FORMS += ui/mainwindow.ui

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
    
    # 窗口子系统 (GUI应用)
    QMAKE_LFLAGS += -Wl,-subsystem,windows
    
    # 添加文件系统库支持
    LIBS += -lstdc++fs
}

# ==================== Linux/macOS 配置 ====================
unix {
    # 添加Crypto++链接
    LIBS += -lcryptopp
    # 添加C++文件系统库
    LIBS += -lstdc++fs
}

# ==================== 编译器标志 ====================
# 启用所有警告
QMAKE_CXXFLAGS += -Wall -Wextra -pedantic
# 禁用特定警告 (可选)
QMAKE_CXXFLAGS += -Wno-deprecated-declarations

# 优化级别
CONFIG(release, debug|release) {
    QMAKE_CXXFLAGS_RELEASE = -O2
}
CONFIG(debug, debug|release) {
    QMAKE_CXXFLAGS_DEBUG = -g
}

# 添加预处理器定义
DEFINES += QT_DEPRECATED_WARNINGS