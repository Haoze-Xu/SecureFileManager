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
#include <QMimeData>
#include <QUrl>
#include <QTreeWidgetItem>
#include <QStyle>
#include <QApplication>
#include <QFileInfoList>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    
    // 设置窗口标题和图标
    setWindowTitle("跨平台文件安全管理系统");
    
    // 配置树形控件
    ui->fileTreeWidget->setHeaderLabels({"文件名", "大小", "类型", "修改日期"});
    ui->fileTreeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui->fileTreeWidget->setSortingEnabled(true);
    
    // 连接添加目录按钮
    connect(ui->addDirectoryButton, &QPushButton::clicked, 
            this, &MainWindow::on_addDirectoryButton_clicked);
    
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
    connect(workerThread, &WorkerThread::logMessageRequested,
            this, &MainWindow::logMessage);
    
    // 初始化最后使用的目录
    lastOutputDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    
    // 启用文件拖拽
    setAcceptDrops(true);
}

MainWindow::~MainWindow()
{
    if (workerThread) {
        workerThread->quit();
        workerThread->wait();
    }
    delete ui;
}

// ==================== 文件操作 ====================

void MainWindow::on_addFilesButton_clicked()
{
    QStringList files = QFileDialog::getOpenFileNames(this, "选择文件", 
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    
    if (!files.isEmpty()) {
        foreach (const QString &file, files) {
            QFileInfo info(file);
            QTreeWidgetItem *item = new QTreeWidgetItem(ui->fileTreeWidget);
            item->setText(0, info.fileName());
            item->setText(1, QString("%1 KB").arg(info.size() / 1024));
            item->setText(2, info.suffix().isEmpty() ? "文件" : info.suffix());
            item->setText(3, info.lastModified().toString("yyyy-MM-dd hh:mm"));
            item->setIcon(0, QApplication::style()->standardIcon(QStyle::SP_FileIcon));
            item->setData(0, Qt::UserRole, info.absoluteFilePath());
            logMessage(QString("添加文件: %1").arg(info.fileName()));
        }
    }
}

void MainWindow::on_addDirectoryButton_clicked()
{
    QString dirPath = QFileDialog::getExistingDirectory(this, "选择目录", 
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    
    if (!dirPath.isEmpty()) {
        loadDirectory(dirPath, nullptr);
        logMessage(QString("添加目录: %1").arg(dirPath));
    }
}

void MainWindow::on_removeSelectedButton_clicked()
{
    QList<QTreeWidgetItem*> items = ui->fileTreeWidget->selectedItems();
    if (items.isEmpty()) {
        logMessage("没有选中的项目", true);
        return;
    }
    
    int count = 0;
    foreach (QTreeWidgetItem *item, items) {
        // 递归删除所有子项
        delete item;
        count++;
    }
    
    logMessage(QString("移除了 %1 个项目").arg(count));
}

void MainWindow::on_clearListButton_clicked()
{
    ui->fileTreeWidget->clear();
    logMessage("清除了文件列表");
}

// ==================== 主要功能 ====================

void MainWindow::on_encryptButton_clicked()
{
    if (ui->fileTreeWidget->topLevelItemCount() == 0) {
        logMessage("请先添加文件或目录", true);
        return;
    }
    
    QString password = ui->passwordLineEdit->text();
    if (password.isEmpty()) {
        logMessage("请输入密码", true);
        return;
    }
    
    // 获取输出目录
    QString outputDir = QFileDialog::getExistingDirectory(this, "选择输出目录", lastOutputDir);
    if (outputDir.isEmpty()) return;
    
    // 保存最后使用的目录
    lastOutputDir = outputDir;
    
    // 准备文件列表
    QList<QString> files = collectSelectedFiles();
    if (files.isEmpty()) {
        logMessage("没有选中的文件", true);
        return;
    }
    
    // 开始加密操作
    updateControlsState(false);
    logMessage(QString("开始加密操作 (%1 个项目)...").arg(files.size()));
    workerThread->processFiles(WorkerThread::Encrypt, files, password, outputDir);
}

void MainWindow::on_decryptButton_clicked()
{
    if (ui->fileTreeWidget->topLevelItemCount() == 0) {
        logMessage("请先添加文件或目录", true);
        return;
    }
    
    QString password = ui->passwordLineEdit->text();
    if (password.isEmpty()) {
        logMessage("请输入密码", true);
        return;
    }
    
    // 获取输出目录
    QString outputDir = QFileDialog::getExistingDirectory(this, "选择输出目录", lastOutputDir);
    if (outputDir.isEmpty()) return;
    
    // 保存最后使用的目录
    lastOutputDir = outputDir;
    
    // 准备文件列表
    QList<QString> files = collectSelectedFiles();
    if (files.isEmpty()) {
        logMessage("没有选中的文件", true);
        return;
    }
    
    updateControlsState(false);
    logMessage(QString("开始解密操作 (%1 个项目)...").arg(files.size()));
    workerThread->processFiles(WorkerThread::Decrypt, files, password, outputDir);
}

void MainWindow::on_wipeButton_clicked()
{
    if (ui->fileTreeWidget->topLevelItemCount() == 0) {
        logMessage("请先添加文件或目录", true);
        return;
    }

    // 获取选中的文件（不包括目录）
    QList<QString> files = collectSelectedFiles();
    if (files.isEmpty()) {
        logMessage("没有选中的文件", true);
        return;
    }
    
    // 危险操作确认对话框
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "危险操作确认",
                                QString("安全擦除将永久删除 %1 个文件，不可恢复！\n确定要继续吗？")
                                .arg(files.size()),
                                QMessageBox::Yes | QMessageBox::No);
    
    if (reply != QMessageBox::Yes) {
        logMessage(QString("安全擦除操作已取消 (%1 个文件)").arg(files.size()));
        return;
    }

    updateControlsState(false);
    logMessage(QString("开始安全擦除操作 (%1 个文件)...").arg(files.size()));
    workerThread->processFiles(WorkerThread::Wipe, files, "");
}

// ==================== 工具功能 ====================

void MainWindow::on_calculateHashButton_clicked()
{
    if (ui->fileTreeWidget->topLevelItemCount() == 0) {
        logMessage("请先添加文件或目录", true);
        return;
    }
    
    QList<QString> files = collectSelectedFiles();
    if (files.isEmpty()) {
        logMessage("没有选中的文件", true);
        return;
    }
    
    updateControlsState(false);
    logMessage(QString("开始计算文件哈希值 (%1 个文件)...").arg(files.size()));
    workerThread->processFiles(WorkerThread::CalculateHash, files, "");
}

void MainWindow::on_showPasswordCheckBox_stateChanged(int state)
{
    ui->passwordLineEdit->setEchoMode(state == Qt::Checked ? 
        QLineEdit::Normal : QLineEdit::Password);
}

// ==================== 取消按钮 ====================

void MainWindow::on_cancelButton_clicked()
{
    if (workerThread) {
        workerThread->cancel();
    }
    logMessage("操作已取消", true);
    updateControlsState(true);
}

// ==================== 线程通信 ====================

void MainWindow::handleProgress(int value, const QString &message)
{
    // 确保进度值在有效范围内
    int progress = qBound(0, value, 100);
    ui->progressBar->setValue(progress);
    
    // 添加详细进度信息
    QString fullMessage;
    if (message.contains("%")) {
        // 如果消息已经包含百分比，直接使用
        fullMessage = message;
    } else {
        // 否则添加百分比
        fullMessage = QString("%1 (%2%)").arg(message).arg(progress);
    }
    
    ui->statusLabel->setText(fullMessage);
    
    // 实时更新日志
    if (progress % 10 == 0 || progress == 100) {
        logMessage(fullMessage);
    }
}

void MainWindow::handleCompleted(bool success, const QString &message)
{
    ui->progressBar->setVisible(false);
    updateControlsState(true);
    
    logMessage(message, !success);
    ui->statusLabel->setText(success ? "操作完成" : "操作失败");
    
    if (success) {
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

// ==================== 日志方法 ====================

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

// ==================== 控件状态更新 ====================

void MainWindow::updateControlsState(bool enabled)
{
    ui->fileTreeWidget->setEnabled(enabled);
    ui->addFilesButton->setEnabled(enabled);
    ui->addDirectoryButton->setEnabled(enabled);
    ui->removeSelectedButton->setEnabled(enabled);
    ui->clearListButton->setEnabled(enabled);
    ui->encryptButton->setEnabled(enabled);
    ui->decryptButton->setEnabled(enabled);
    ui->wipeButton->setEnabled(enabled);
    ui->calculateHashButton->setEnabled(enabled);
    ui->passwordLineEdit->setEnabled(enabled);
    ui->showPasswordCheckBox->setEnabled(enabled);
    
    ui->cancelButton->setEnabled(!enabled);
    
    ui->progressBar->setVisible(!enabled);
    if (!enabled) {
        ui->progressBar->setValue(0);
    }
}

// ==================== 文件拖拽支持 ====================

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();
    if (mimeData->hasUrls()) {
        QList<QUrl> urlList = mimeData->urls();
        int count = 0;
        
        for (const QUrl &url : urlList) {
            if (url.isLocalFile()) {
                QFileInfo info(url.toLocalFile());
                if (info.isDir()) {
                    loadDirectory(info.absoluteFilePath(), nullptr);
                    count++;
                } else {
                    // 添加单个文件
                    QTreeWidgetItem *item = new QTreeWidgetItem(ui->fileTreeWidget);
                    item->setText(0, info.fileName());
                    item->setText(1, QString("%1 KB").arg(info.size() / 1024));
                    item->setText(2, info.suffix().isEmpty() ? "文件" : info.suffix());
                    item->setText(3, info.lastModified().toString("yyyy-MM-dd hh:mm"));
                    item->setIcon(0, QApplication::style()->standardIcon(QStyle::SP_FileIcon));
                    item->setData(0, Qt::UserRole, info.absoluteFilePath());
                    count++;
                }
            }
        }
        
        if (count > 0) {
            logMessage(QString("拖拽添加了 %1 个项目").arg(count));
        }
    }
}

// ==================== 树形目录功能 ====================

void MainWindow::loadDirectory(const QString &path, QTreeWidgetItem *parent)
{
    QDir dir(path);
    if (!dir.exists()) {
        logMessage(QString("目录不存在: %1").arg(path), true);
        return;
    }

    // 创建目录节点
    QTreeWidgetItem *dirItem = new QTreeWidgetItem(parent);
    dirItem->setText(0, dir.dirName());
    dirItem->setIcon(0, QApplication::style()->standardIcon(QStyle::SP_DirIcon));
    dirItem->setData(0, Qt::UserRole, path);
    dirItem->setFlags(dirItem->flags() | Qt::ItemIsSelectable);
    
    if (!parent) {
        ui->fileTreeWidget->addTopLevelItem(dirItem);
    }
    
    // 添加文件
    QFileInfoList fileList = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    for (const QFileInfo &info : fileList) {
        QTreeWidgetItem *fileItem = new QTreeWidgetItem(dirItem);
        fileItem->setText(0, info.fileName());
        fileItem->setText(1, QString("%1 KB").arg(info.size() / 1024));
        fileItem->setText(2, info.suffix().isEmpty() ? "文件" : info.suffix());
        fileItem->setText(3, info.lastModified().toString("yyyy-MM-dd hh:mm"));
        fileItem->setIcon(0, QApplication::style()->standardIcon(QStyle::SP_FileIcon));
        fileItem->setData(0, Qt::UserRole, info.absoluteFilePath());
        fileItem->setFlags(fileItem->flags() | Qt::ItemIsSelectable);
    }
    
    // 递归添加子目录
    QFileInfoList dirList = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &subDir : dirList) {
        loadDirectory(subDir.absoluteFilePath(), dirItem);
    }
    
    dirItem->setExpanded(true);
}

QList<QString> MainWindow::collectSelectedFiles()
{
    QList<QString> files;
    QList<QTreeWidgetItem*> items = ui->fileTreeWidget->selectedItems();
    
    for (QTreeWidgetItem *item : items) {
        collectFilesFromItem(item, files);
    }
    
    return files;
}

void MainWindow::collectFilesFromItem(QTreeWidgetItem *item, QList<QString> &files)
{
    // 检查是否是文件节点
    if (item->childCount() == 0) {
        QString filePath = item->data(0, Qt::UserRole).toString();
        if (!filePath.isEmpty()) {
            files.append(filePath);
        }
    } 
    // 如果是目录节点，递归处理所有子项
    else {
        for (int i = 0; i < item->childCount(); i++) {
            collectFilesFromItem(item->child(i), files);
        }
    }
}

// ==================== WorkerThread 实现 ====================

WorkerThread::WorkerThread(QObject *parent) 
    : QThread(parent), m_cancel(false) 
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
    m_cancel = false;
    try {
        if (m_cancel) {
            emit operationCompleted(false, "操作已取消");
            return;
        }
        
        const int totalFiles = fileList.size();
        int processedFiles = 0;
        
        foreach (const QString &path, fileList) {
            if (m_cancel) {
                emit operationCompleted(false, "操作已取消");
                return;
            }
            
            QFileInfo info(path);
            if (info.isDir()) {
                processDirectory(currentOp, path);
            } else {
                processSingleFile(currentOp, path);
            }
            
            processedFiles++;
            int progress = static_cast<int>((processedFiles * 100) / totalFiles);
            emit progressChanged(progress, 
                QString("已完成 %1/%2").arg(processedFiles).arg(totalFiles));
        }
        
        emit operationCompleted(true, "所有操作成功完成");
    } catch (const std::exception &e) {
        emit operationCompleted(false, QString("操作失败: %1").arg(e.what()));
    }
}

void WorkerThread::processDirectory(Operation op, const QString &dirPath)
{
    QDir dir(dirPath);
    if (!dir.exists()) {
        emit logMessageRequested(QString("目录不存在: %1").arg(dirPath), true);
        return;
    }
    
    // 获取目录下所有文件和子目录
    QFileInfoList entries = dir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
    int totalEntries = entries.size();
    int processed = 0;
    
    for (const QFileInfo &entry : entries) {
        if (m_cancel) return;
        
        processed++;
        int progress = static_cast<int>((processed * 100) / totalEntries);
        emit progressChanged(progress, QString("处理目录: %1 (%2/%3)")
                                .arg(dir.dirName())
                                .arg(processed)
                                .arg(totalEntries));
        
        if (entry.isFile()) {
            processSingleFile(op, entry.absoluteFilePath());
        } else if (entry.isDir()) {
            processDirectory(op, entry.absoluteFilePath());
        }
    }
}

void WorkerThread::processSingleFile(Operation op, const QString &filePath)
{
    QFileInfo fileInfo(filePath);
    QString outputPath;
    
    // 更新处理状态
    emit progressChanged(0, QString("正在处理: %1").arg(fileInfo.fileName()));
    
    try {
        // 定义进度回调
        auto progressCallback = [this, fileInfo](int progress) {
            QString msg = QString("处理 %1: %2%")
                         .arg(fileInfo.fileName())
                         .arg(progress);
            emit progressChanged(progress, msg);
        };
        
        if (op == Encrypt) {
            // 确定输出路径
            outputPath = outputDirectory + "/" + fileInfo.fileName() + ".enc";
            
            CryptoEngine::encryptFile(
                filePath.toStdString(), 
                outputPath.toStdString(), 
                password.toStdString(),
                progressCallback
            );
            
            // 添加详细的加密成功日志
            QFileInfo outInfo(outputPath);
            QString successMsg = QString("加密成功! 输出文件: %1 (大小: %2 字节)")
                                .arg(outInfo.absoluteFilePath())
                                .arg(outInfo.size());
            emit logMessageRequested(successMsg);
        } 
        else if (op == Decrypt) {
            // 解密前的文件验证
            if (!CryptoEngine::isEncryptedFile(filePath.toStdString())) {
                QString errorMsg = QString("'%1' 不是有效的加密文件，跳过")
                                    .arg(fileInfo.fileName());
                emit logMessageRequested(errorMsg, true);
                return;
            }
            
            // 确定输出路径
            QString baseName = fileInfo.fileName();
            if (baseName.endsWith(".enc")) {
                baseName.chop(4);
            }
            outputPath = outputDirectory + "/decrypted_" + baseName;
            
            CryptoEngine::decryptFile(
                filePath.toStdString(), 
                outputPath.toStdString(), 
                password.toStdString(),
                progressCallback
            );
            
            // 添加详细的解密成功日志
            QFileInfo outInfo(outputPath);
            QString successMsg = QString("解密成功! 输出文件: %1 (大小: %2 字节)")
                                .arg(outInfo.absoluteFilePath())
                                .arg(outInfo.size());
            emit logMessageRequested(successMsg);
        }
        else if (op == Wipe) {
            bool success = FileProcessor::secureDelete(filePath.toStdString());
            if (!success) {
                throw std::runtime_error("安全擦除操作失败");
            }
            
            // 添加详细的擦除确认
            QString successMsg = QString("已安全擦除: %1 (永久删除)")
                                .arg(fileInfo.absoluteFilePath());
            emit logMessageRequested(successMsg);
        }
        else if (op == CalculateHash) {
            std::string hashValue = FileProcessor::calculateSHA256(filePath.toStdString());
            QString result = QString("%1 的 SHA-256: %2")
                .arg(fileInfo.fileName(), QString::fromStdString(hashValue));
            
            emit logMessageRequested(result);
        }
        
        // 标记文件已处理
        emit fileProcessed(filePath);
    } 
    catch (const std::exception &e) {
        // 处理异常
        QString errorMsg = QString("处理文件 %1 时出错: %2")
            .arg(fileInfo.fileName(), e.what());
        emit logMessageRequested(errorMsg, true);
    }
}