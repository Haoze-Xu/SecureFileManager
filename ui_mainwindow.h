/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.9.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QVBoxLayout *verticalLayout;
    QGroupBox *groupBox;
    QVBoxLayout *verticalLayout_2;
    QHBoxLayout *horizontalLayout;
    QPushButton *addFilesButton;
    QPushButton *removeSelectedButton;
    QPushButton *clearListButton;
    QSpacerItem *horizontalSpacer;
    QListWidget *fileListWidget;
    QGroupBox *groupBox_2;
    QVBoxLayout *verticalLayout_3;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label;
    QLineEdit *passwordLineEdit;
    QCheckBox *showPasswordCheckBox;
    QGroupBox *groupBox_3;
    QGridLayout *gridLayout;
    QPushButton *encryptButton;
    QPushButton *decryptButton;
    QPushButton *wipeButton;
    QPushButton *calculateHashButton;
    QGroupBox *groupBox_4;
    QVBoxLayout *verticalLayout_4;
    QProgressBar *progressBar;
    QLabel *statusLabel;
    QGroupBox *groupBox_5;
    QVBoxLayout *verticalLayout_5;
    QTextEdit *logTextEdit;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(800, 600);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        verticalLayout = new QVBoxLayout(centralwidget);
        verticalLayout->setObjectName("verticalLayout");
        groupBox = new QGroupBox(centralwidget);
        groupBox->setObjectName("groupBox");
        verticalLayout_2 = new QVBoxLayout(groupBox);
        verticalLayout_2->setObjectName("verticalLayout_2");
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        addFilesButton = new QPushButton(groupBox);
        addFilesButton->setObjectName("addFilesButton");

        horizontalLayout->addWidget(addFilesButton);

        removeSelectedButton = new QPushButton(groupBox);
        removeSelectedButton->setObjectName("removeSelectedButton");

        horizontalLayout->addWidget(removeSelectedButton);

        clearListButton = new QPushButton(groupBox);
        clearListButton->setObjectName("clearListButton");

        horizontalLayout->addWidget(clearListButton);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);


        verticalLayout_2->addLayout(horizontalLayout);

        fileListWidget = new QListWidget(groupBox);
        fileListWidget->setObjectName("fileListWidget");
        fileListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);

        verticalLayout_2->addWidget(fileListWidget);


        verticalLayout->addWidget(groupBox);

        groupBox_2 = new QGroupBox(centralwidget);
        groupBox_2->setObjectName("groupBox_2");
        verticalLayout_3 = new QVBoxLayout(groupBox_2);
        verticalLayout_3->setObjectName("verticalLayout_3");
        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        label = new QLabel(groupBox_2);
        label->setObjectName("label");

        horizontalLayout_2->addWidget(label);

        passwordLineEdit = new QLineEdit(groupBox_2);
        passwordLineEdit->setObjectName("passwordLineEdit");

        horizontalLayout_2->addWidget(passwordLineEdit);

        showPasswordCheckBox = new QCheckBox(groupBox_2);
        showPasswordCheckBox->setObjectName("showPasswordCheckBox");

        horizontalLayout_2->addWidget(showPasswordCheckBox);


        verticalLayout_3->addLayout(horizontalLayout_2);


        verticalLayout->addWidget(groupBox_2);

        groupBox_3 = new QGroupBox(centralwidget);
        groupBox_3->setObjectName("groupBox_3");
        gridLayout = new QGridLayout(groupBox_3);
        gridLayout->setObjectName("gridLayout");
        encryptButton = new QPushButton(groupBox_3);
        encryptButton->setObjectName("encryptButton");

        gridLayout->addWidget(encryptButton, 0, 0, 1, 1);

        decryptButton = new QPushButton(groupBox_3);
        decryptButton->setObjectName("decryptButton");

        gridLayout->addWidget(decryptButton, 0, 1, 1, 1);

        wipeButton = new QPushButton(groupBox_3);
        wipeButton->setObjectName("wipeButton");

        gridLayout->addWidget(wipeButton, 0, 2, 1, 1);

        calculateHashButton = new QPushButton(groupBox_3);
        calculateHashButton->setObjectName("calculateHashButton");

        gridLayout->addWidget(calculateHashButton, 0, 3, 1, 1);


        verticalLayout->addWidget(groupBox_3);

        groupBox_4 = new QGroupBox(centralwidget);
        groupBox_4->setObjectName("groupBox_4");
        verticalLayout_4 = new QVBoxLayout(groupBox_4);
        verticalLayout_4->setObjectName("verticalLayout_4");
        progressBar = new QProgressBar(groupBox_4);
        progressBar->setObjectName("progressBar");
        progressBar->setValue(0);

        verticalLayout_4->addWidget(progressBar);

        statusLabel = new QLabel(groupBox_4);
        statusLabel->setObjectName("statusLabel");

        verticalLayout_4->addWidget(statusLabel);


        verticalLayout->addWidget(groupBox_4);

        groupBox_5 = new QGroupBox(centralwidget);
        groupBox_5->setObjectName("groupBox_5");
        verticalLayout_5 = new QVBoxLayout(groupBox_5);
        verticalLayout_5->setObjectName("verticalLayout_5");
        logTextEdit = new QTextEdit(groupBox_5);
        logTextEdit->setObjectName("logTextEdit");
        logTextEdit->setReadOnly(true);

        verticalLayout_5->addWidget(logTextEdit);


        verticalLayout->addWidget(groupBox_5);

        MainWindow->setCentralWidget(centralwidget);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "\350\267\250\345\271\263\345\217\260\346\226\207\344\273\266\345\256\211\345\205\250\347\256\241\347\220\206\347\263\273\347\273\237", nullptr));
        groupBox->setTitle(QCoreApplication::translate("MainWindow", "\346\226\207\344\273\266\345\210\227\350\241\250", nullptr));
        addFilesButton->setText(QCoreApplication::translate("MainWindow", "\346\267\273\345\212\240\346\226\207\344\273\266", nullptr));
        removeSelectedButton->setText(QCoreApplication::translate("MainWindow", "\347\247\273\351\231\244\351\200\211\344\270\255", nullptr));
        clearListButton->setText(QCoreApplication::translate("MainWindow", "\346\270\205\347\251\272\345\210\227\350\241\250", nullptr));
        groupBox_2->setTitle(QCoreApplication::translate("MainWindow", "\345\256\211\345\205\250\350\256\276\347\275\256", nullptr));
        label->setText(QCoreApplication::translate("MainWindow", "\345\257\206\347\240\201:", nullptr));
        showPasswordCheckBox->setText(QCoreApplication::translate("MainWindow", "\346\230\276\347\244\272\345\257\206\347\240\201", nullptr));
        groupBox_3->setTitle(QCoreApplication::translate("MainWindow", "\346\223\215\344\275\234", nullptr));
        encryptButton->setText(QCoreApplication::translate("MainWindow", "\345\212\240\345\257\206\346\226\207\344\273\266", nullptr));
        decryptButton->setText(QCoreApplication::translate("MainWindow", "\350\247\243\345\257\206\346\226\207\344\273\266", nullptr));
        wipeButton->setText(QCoreApplication::translate("MainWindow", "\345\256\211\345\205\250\346\223\246\351\231\244", nullptr));
        calculateHashButton->setText(QCoreApplication::translate("MainWindow", "\350\256\241\347\256\227\345\223\210\345\270\214", nullptr));
        groupBox_4->setTitle(QCoreApplication::translate("MainWindow", "\346\223\215\344\275\234\347\212\266\346\200\201", nullptr));
        statusLabel->setText(QCoreApplication::translate("MainWindow", "\345\260\261\347\273\252", nullptr));
        groupBox_5->setTitle(QCoreApplication::translate("MainWindow", "\346\223\215\344\275\234\346\227\245\345\277\227", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
