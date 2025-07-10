#include "../include/mainwindow.h"
#include "ui_mainwindow.h"
#include "../include/file_processor.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDateTime>
#include <QTextCursor>
#include <QScrollBar>
#include <QDir>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    
    // 设置窗口标题和图标
    setWindowTitle("跨平台文件安全管理系统");
    
    // 初始化状态
    ui->progressBar->setVisible(false);
    ui->progressBar->setRange(0, 100);
    ui->statusLabel->setText("就绪");
    
    // 设置密码输入框
    ui->passwordLineEdit->setEchoMode(QLineEdit::Password);
    
    // 创建并连接工作线程
    workerThread = new WorkerThread(this);
    connect(workerThread, &WorkerThread::progressChanged, 
            this, &MainWindow::handleProgress);
    connect(workerThread, &WorkerThread::operationCompleted, 
            this, &MainWindow::handleCompleted);
    connect(workerThread, &WorkerThread::fileProcessed, 
            this, &MainWindow::handleFileProcessed);
            
    // 修复：添加日志信号连接
    connect(workerThread, &WorkerThread::logMessageRequested,
            this, &MainWindow::logMessage);
}

MainWindow::~MainWindow()
{
    workerThread->quit();
    workerThread->wait();
    delete ui;
}

void MainWindow::on_addFilesButton_clicked()
{
    QStringList files = QFileDialog::getOpenFileNames(this, "选择文件", 
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    
    if (!files.isEmpty()) {
        ui->fileListWidget->addItems(files);

        // ==== 增强日志：显示每个添加的文件名 ====
        foreach (const QString &file, files) {
            logMessage(QString("添加文件: %1").arg(QFileInfo(file).fileName()));
        }
    }
}

void MainWindow::on_removeSelectedButton_clicked()
{
    QList<QListWidgetItem*> items = ui->fileListWidget->selectedItems();
    if (items.isEmpty()) {
        logMessage("没有选中的文件", true);
        return;
    }
    
    qDeleteAll(items);
    logMessage(QString("移除了 %1 个文件").arg(items.size()));
}

void MainWindow::on_clearListButton_clicked()
{
    ui->fileListWidget->clear();
    logMessage("清除了文件列表");
}

void MainWindow::on_encryptButton_clicked()
{
    if (ui->fileListWidget->count() == 0) {
        logMessage("请先添加文件", true);
        return;
    }
    
    QString password = ui->passwordLineEdit->text();
    if (password.isEmpty()) {
        logMessage("请输入密码", true);
        return;
    }
    
    // 获取输出目录
    QString outputDir = QFileDialog::getExistingDirectory(this, "选择输出目录", 
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    
    if (outputDir.isEmpty()) return;
    
    // 准备文件列表
    QList<QString> files;
    for (int i = 0; i < ui->fileListWidget->count(); i++) {
        files.append(ui->fileListWidget->item(i)->text());
    }
    
    // 开始加密操作
    updateControlsState(false);
    logMessage("开始加密操作...");
    workerThread->processFiles(WorkerThread::Encrypt, files, password, outputDir);
}

void MainWindow::on_decryptButton_clicked()
{
    // 类似加密操作
    if (ui->fileListWidget->count() == 0) {
        logMessage("请先添加文件", true);
        return;
    }
    
    QString password = ui->passwordLineEdit->text();
    if (password.isEmpty()) {
        logMessage("请输入密码", true);
        return;
    }
    
    QString outputDir = QFileDialog::getExistingDirectory(this, "选择输出目录", 
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    
    if (outputDir.isEmpty()) return;
    
    QList<QString> files;
    for (int i = 0; i < ui->fileListWidget->count(); i++) {
        files.append(ui->fileListWidget->item(i)->text());
    }
    
    updateControlsState(false);
    logMessage("开始解密操作...");
    workerThread->processFiles(WorkerThread::Decrypt, files, password, outputDir);
}

void MainWindow::on_wipeButton_clicked()
{
    if (ui->fileListWidget->count() == 0) {
        logMessage("请先添加文件", true);
        return;
    }

    // ==== 添加危险操作确认对话框 ====
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "危险操作确认",
                                QString("安全擦除将永久删除 %1 个文件，不可恢复！\n确定要继续吗？")
                                .arg(ui->fileListWidget->count()),
                                QMessageBox::Yes | QMessageBox::No);
    
    if (reply != QMessageBox::Yes) {
        logMessage(QString("安全擦除操作已取消 (%1 个文件)").arg(ui->fileListWidget->count()));
        return;
    }
    // ==============================

    QList<QString> files;
    for (int i = 0; i < ui->fileListWidget->count(); i++) {
        files.append(ui->fileListWidget->item(i)->text());
    }
    
    updateControlsState(false);
    logMessage("开始安全擦除操作...");
    workerThread->processFiles(WorkerThread::Wipe, files, "");
}

void MainWindow::on_calculateHashButton_clicked()
{
    if (ui->fileListWidget->count() == 0) {
        logMessage("请先添加文件", true);
        return;
    }
    
    QList<QString> files;
    for (int i = 0; i < ui->fileListWidget->count(); i++) {
        files.append(ui->fileListWidget->item(i)->text());
    }
    
    updateControlsState(false);
    logMessage("开始计算文件哈希值...");
    workerThread->processFiles(WorkerThread::CalculateHash, files, "");
}

void MainWindow::on_showPasswordCheckBox_stateChanged(int state)
{
    ui->passwordLineEdit->setEchoMode(state == Qt::Checked ? 
        QLineEdit::Normal : QLineEdit::Password);
}

void MainWindow::handleProgress(int value, const QString &message)
{
    ui->progressBar->setValue(value);
    ui->statusLabel->setText(message);
}

void MainWindow::handleCompleted(bool success, const QString &message)
{
    ui->progressBar->setVisible(false);
    updateControlsState(true);
    
    logMessage(message, !success);
    ui->statusLabel->setText(success ? "操作完成" : "操作失败");
    
    if (success) {
        // 使用公共访问器获取操作类型
        WorkerThread::Operation op = workerThread->currentOperation();
        
        QString detailMessage;
        if (op == WorkerThread::Encrypt) {
            detailMessage = "加密操作完成！所有文件已成功加密";
        } else if (op == WorkerThread::Decrypt) {
            detailMessage = "解密操作完成！所有文件已成功解密";
        } else if (op == WorkerThread::Wipe) {
            detailMessage = "安全擦除完成！所有文件已永久删除";
        } else if (op == WorkerThread::CalculateHash) {
            detailMessage = "哈希计算完成！结果已显示在日志中";
        }
        
        QMessageBox::information(this, "操作完成", detailMessage);
    } else {
        QMessageBox::critical(this, "操作失败", message);
    }
}

void MainWindow::handleFileProcessed(const QString &filename)
{
    logMessage(QString("已处理: %1").arg(QFileInfo(filename).fileName()));
}

void MainWindow::logMessage(const QString &message, bool isError)
{
    QString timestamp = QDateTime::currentDateTime().toString("[hh:mm:ss] ");
    QString htmlMessage;
    
    if (isError) {
        htmlMessage = QString("<span style='color:red;'>%1%2</span>")
                         .arg(timestamp, message);
    } else {
        htmlMessage = QString("<span style='color:blue;'>%1%2</span>")
                         .arg(timestamp, message);
    }
    
    // 添加消息到日志区域
    QTextCursor cursor(ui->logTextEdit->document());
    cursor.movePosition(QTextCursor::End);
    cursor.insertHtml(htmlMessage + "<br>");
    
    // 滚动到底部
    ui->logTextEdit->verticalScrollBar()->setValue(
        ui->logTextEdit->verticalScrollBar()->maximum());
}

void MainWindow::updateControlsState(bool enabled)
{
    ui->fileListWidget->setEnabled(enabled);
    ui->addFilesButton->setEnabled(enabled);
    ui->removeSelectedButton->setEnabled(enabled);
    ui->clearListButton->setEnabled(enabled);
    ui->encryptButton->setEnabled(enabled);
    ui->decryptButton->setEnabled(enabled);
    ui->wipeButton->setEnabled(enabled);
    ui->calculateHashButton->setEnabled(enabled);
    ui->passwordLineEdit->setEnabled(enabled);
    ui->showPasswordCheckBox->setEnabled(enabled);
    
    ui->progressBar->setVisible(!enabled);
    if (!enabled) {
        ui->progressBar->setValue(0);
    }
}

// ==================== WorkerThread 实现 ====================

WorkerThread::WorkerThread(QObject *parent) 
    : QThread(parent) 
{
}

void WorkerThread::processFiles(Operation op, const QList<QString> &files, 
                               const QString &pwd, const QString &outDir)
{
    currentOp = op;
    fileList = files;
    password = pwd;
    outputDirectory = outDir;
    start();
}

void WorkerThread::run()
{
    try {
        switch (currentOp) {
        case Encrypt:
        case Decrypt:
            processEncryptDecrypt();
            break;
        case Wipe:
            processWipe();
            break;
        case CalculateHash:
            processHash();
            break;
        }
        
        emit operationCompleted(true, "所有操作成功完成");
    } catch (const std::exception &e) {
        emit operationCompleted(false, QString("操作失败: %1").arg(e.what()));
    }
}

void WorkerThread::processEncryptDecrypt()
{
    const int totalFiles = fileList.size();
    int processed = 0;
    
    foreach (const QString &filePath, fileList) {
        QFileInfo fileInfo(filePath);
        QString outputPath;

        // 解密前的文件验证
        if (currentOp == Decrypt) {
            if (!CryptoEngine::isEncryptedFile(filePath.toStdString())) {
                QString errorMsg = QString("'%1' 不是有效的加密文件")
                                    .arg(fileInfo.fileName()); // 使用当前文件名
                emit progressChanged(0, errorMsg);
                throw std::runtime_error(errorMsg.toStdString());
            }
        }
        
        // 确定输出路径
        if (currentOp == Encrypt) {
            outputPath = outputDirectory + "/" + fileInfo.fileName() + ".enc";
        } else {
            QString baseName = fileInfo.fileName();
            if (baseName.endsWith(".enc")) {
                baseName.chop(4);
            }
            outputPath = outputDirectory + "/decrypted_" + baseName;
        }
        
        // 更新处理状态
        emit progressChanged(0, QString("正在处理: %1").arg(fileInfo.fileName()));
        
        try {
            // 执行加密/解密操作
            if (currentOp == Encrypt) {
                CryptoEngine::encryptFile(
                    filePath.toStdString(), 
                    outputPath.toStdString(), 
                    password.toStdString()
                );
                
                // 添加详细的加密成功日志
                QFileInfo outInfo(outputPath);
                QString successMsg = QString("加密成功! 输出文件: %1 (大小: %2 字节)")
                                    .arg(outInfo.fileName())
                                    .arg(outInfo.size());
                emit progressChanged(0, successMsg);
            } else {
                CryptoEngine::decryptFile(
                    filePath.toStdString(), 
                    outputPath.toStdString(), 
                    password.toStdString()
                );
                
                // 添加详细的解密成功日志
                QFileInfo outInfo(outputPath);
                QString successMsg = QString("解密成功! 输出文件: %1 (大小: %2 字节)")
                                    .arg(outInfo.fileName())
                                    .arg(outInfo.size());
                emit progressChanged(0, successMsg);
            }
            
            // 标记文件已处理
            emit fileProcessed(filePath);
        } 
        catch (const std::exception &e) {
            // 处理异常
            throw std::runtime_error(
                QString("处理文件 %1 时出错: %2")
                    .arg(fileInfo.fileName(), e.what())
                    .toStdString()
            );
        }
        
        // 更新进度
        processed++;
        int progress = static_cast<int>((processed * 100) / totalFiles);
        emit progressChanged(progress, 
            QString("已完成 %1/%2").arg(processed).arg(totalFiles));
    }
}

void WorkerThread::processWipe()
{
    const int totalFiles = fileList.size();
    int processed = 0;
    
    foreach (const QString &filePath, fileList) {
        QFileInfo fileInfo(filePath);
        emit progressChanged(0, QString("正在安全擦除: %1").arg(fileInfo.fileName()));
        
        try {
            bool success = FileProcessor::secureDelete(filePath.toStdString());
            if (!success) {
                throw std::runtime_error("安全擦除操作失败");
            }
            
            // ==== 增强日志：明确擦除结果 ====
            QString successMsg = QString("已安全擦除: %1 (永久删除)").arg(fileInfo.fileName());
            QMetaObject::invokeMethod(parent(), "logMessage", 
                                     Q_ARG(QString, successMsg));
            
            emit fileProcessed(filePath);
        } catch (const std::exception &e) {
            throw std::runtime_error(
                QString("擦除文件 %1 时出错: %2")
                    .arg(fileInfo.fileName(), e.what())
                    .toStdString()
            );
        }
        
        processed++;
        int progress = static_cast<int>((processed * 100) / totalFiles);
        emit progressChanged(progress, 
            QString("已完成 %1/%2").arg(processed).arg(totalFiles));
    }
    
    // ==== 添加操作完成信息 ====
    emit operationCompleted(true, QString("安全擦除完成！已永久删除 %1 个文件").arg(totalFiles));
}

void WorkerThread::processHash() {
    const int totalFiles = fileList.size();
    int processed = 0;
    
    foreach (const QString &filePath, fileList) {
        QFileInfo fileInfo(filePath);
        emit progressChanged(0, QString("正在计算哈希: %1").arg(fileInfo.fileName()));
        
        try {
            // 实际计算哈希值
            std::string hashValue = FileProcessor::calculateSHA256(filePath.toStdString());
            QString result = QString("%1 的 SHA-256: %2")
                .arg(fileInfo.fileName(), QString::fromStdString(hashValue));
            
            // ==== 修复：使用正确的信号 ====
            emit logMessageRequested(result);
            
            emit fileProcessed(filePath);
        } catch (const std::exception &e) {
            throw std::runtime_error(
                QString("计算文件 %1 的哈希时出错: %2")
                    .arg(fileInfo.fileName(), e.what())
                    .toStdString()
            );
        }
        
        processed++;
        int progress = static_cast<int>((processed * 100) / totalFiles);
        emit progressChanged(progress, 
            QString("已完成 %1/%2").arg(processed).arg(totalFiles));
    }
}