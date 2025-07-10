#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QList>
#include <QProgressBar>
#include <QThread>
#include <QDragEnterEvent>
#include <QDropEvent>
#include "../include/crypto_engine.h"
#include "../include/file_processor.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class WorkerThread;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    // 文件操作
    void on_addFilesButton_clicked();
    void on_removeSelectedButton_clicked();
    void on_clearListButton_clicked();
    
    // 主要功能
    void on_encryptButton_clicked();
    void on_decryptButton_clicked();
    void on_wipeButton_clicked();
    
    // 工具功能
    void on_calculateHashButton_clicked();
    void on_showPasswordCheckBox_stateChanged(int state);
    
    // 取消按钮
    void on_cancelButton_clicked();
    
    // 线程通信
    void handleProgress(int value, const QString &message);
    void handleCompleted(bool success, const QString &message);
    void handleFileProcessed(const QString &filename);
    
    // 日志方法
    void logMessage(const QString &message, bool isError = false);

private:
    Ui::MainWindow *ui;
    WorkerThread *workerThread;
    QString lastOutputDir;
    
    void updateControlsState(bool enabled);
};

class WorkerThread : public QThread
{
    Q_OBJECT
public:
    enum Operation { 
        Encrypt, 
        Decrypt, 
        Wipe,
        CalculateHash
    };
    
    explicit WorkerThread(QObject *parent = nullptr);
    void processFiles(Operation op, const QList<QString> &files, 
                     const QString &password, const QString &outputDir = "");
    
    Operation currentOperation() const { return currentOp; }
    void cancel() { m_cancel = true; }

signals:
    void progressChanged(int value, const QString &message);
    void operationCompleted(bool success, const QString &message);
    void fileProcessed(const QString &filename);
    void logMessageRequested(const QString &message, bool isError = false);
    
protected:
    void run() override;
    
private:
    Operation currentOp;
    QList<QString> fileList;
    QString password;
    QString outputDirectory;
    std::atomic<bool> m_cancel;
    
    void processEncryptDecrypt();
    void processWipe();
    void processHash();
};

#endif // MAINWINDOW_H