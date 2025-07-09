#include "../include/mainwindow.h"
#include "ui_mainwindow.h"
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
        logMessage(QString("添加了 %1 个文件").arg(files.size()));
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
        QMessageBox::information(this, "操作完成", message);
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
        
        if (currentOp == Encrypt) {
            outputPath = outputDirectory + "/" + fileInfo.fileName() + ".enc";
        } else {
            QString baseName = fileInfo.fileName();
            if (baseName.endsWith(".enc")) {
                baseName.chop(4);
            }
            outputPath = outputDirectory + "/decrypted_" + baseName;
        }
        
        emit progressChanged(0, QString("正在处理: %1").arg(fileInfo.fileName()));
        
        try {
            if (currentOp == Encrypt) {
                CryptoEngine::encryptFile(
                    filePath.toStdString(), 
                    outputPath.toStdString(), 
                    password.toStdString()
                );
            } else {
                CryptoEngine::decryptFile(
                    filePath.toStdString(), 
                    outputPath.toStdString(), 
                    password.toStdString()
                );
            }
            
            emit fileProcessed(filePath);
        } catch (const std::exception &e) {
            throw std::runtime_error(
                QString("处理文件 %1 时出错: %2")
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
}

void WorkerThread::processHash()
{
    // 哈希计算实现 (后续添加)
    // 暂时模拟实现
    const int totalFiles = fileList.size();
    int processed = 0;
    
    foreach (const QString &filePath, fileList) {
        QFileInfo fileInfo(filePath);
        emit progressChanged(0, QString("正在计算哈希: %1").arg(fileInfo.fileName()));
        
        // 模拟计算耗时
        QThread::msleep(500);
        
        // TODO: 实际计算哈希值
        QString hashValue = "模拟哈希值: 7f83b1657ff1fc53b92dc18148a1d65dfc2d4b1fa3d677284addd200126d9069";
        
        emit progressChanged(0, QString("%1 的 SHA-256: %2")
            .arg(fileInfo.fileName(), hashValue));
        emit fileProcessed(filePath);
        
        processed++;
        int progress = static_cast<int>((processed * 100) / totalFiles);
        emit progressChanged(progress, 
            QString("已完成 %1/%2").arg(processed).arg(totalFiles));
    }
}