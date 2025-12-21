/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.9.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFrame>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QFrame *frame;
    QTextEdit *inputCode;
    QLabel *label_2;
    QLabel *label_3;
    QTextEdit *parallelCode;
    QTextEdit *outputLog;
    QSpinBox *procCount;
    QLabel *label_5;
    QPushButton *runButton;
    QLabel *label_8;
    QSpinBox *procCount_3;
    QTextEdit *outputLog_3;
    QLabel *label_9;
    QTextEdit *outputLog_4;
    QLabel *label_10;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(1525, 761);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        frame = new QFrame(centralwidget);
        frame->setObjectName("frame");
        frame->setGeometry(QRect(0, 0, 1531, 721));
        QPalette palette;
        QBrush brush(QColor(43, 45, 48, 255));
        brush.setStyle(Qt::BrushStyle::SolidPattern);
        palette.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Button, brush);
        palette.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Base, brush);
        palette.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Window, brush);
        palette.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Button, brush);
        palette.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Base, brush);
        palette.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Window, brush);
        palette.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Button, brush);
        palette.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Base, brush);
        palette.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Window, brush);
        frame->setPalette(palette);
        frame->setAutoFillBackground(false);
        frame->setStyleSheet(QString::fromUtf8("\n"
"background-color: rgb(43, 45, 48);"));
        frame->setFrameShape(QFrame::Shape::StyledPanel);
        frame->setFrameShadow(QFrame::Shadow::Raised);
        inputCode = new QTextEdit(frame);
        inputCode->setObjectName("inputCode");
        inputCode->setGeometry(QRect(20, 50, 471, 271));
        QFont font;
        font.setFamilies({QString::fromUtf8("Segoe UI")});
        font.setPointSize(10);
        font.setBold(false);
        font.setItalic(false);
        inputCode->setFont(font);
        inputCode->setAutoFillBackground(false);
        inputCode->setStyleSheet(QString::fromUtf8("background-color: rgb(0, 0, 0);\n"
"font: 10pt \"Segoe UI\";\n"
"color: rgb(255, 255, 255);"));
        label_2 = new QLabel(frame);
        label_2->setObjectName("label_2");
        label_2->setGeometry(QRect(540, 10, 151, 21));
        label_2->setStyleSheet(QString::fromUtf8("color: rgb(255, 255, 255);\n"
"background-color: rgb(43, 45, 48);\n"
"font: 700 12pt \"Segoe UI\";"));
        label_3 = new QLabel(frame);
        label_3->setObjectName("label_3");
        label_3->setGeometry(QRect(540, 260, 71, 21));
        label_3->setStyleSheet(QString::fromUtf8("color: rgb(255, 255, 255);\n"
"background-color: rgb(43, 45, 48);\n"
"font: 700 12pt \"Segoe UI\";"));
        parallelCode = new QTextEdit(frame);
        parallelCode->setObjectName("parallelCode");
        parallelCode->setGeometry(QRect(540, 50, 501, 191));
        parallelCode->setFont(font);
        parallelCode->setAutoFillBackground(false);
        parallelCode->setStyleSheet(QString::fromUtf8("background-color: rgb(0, 0, 0);\n"
"font: 10pt \"Segoe UI\";\n"
"color: rgb(255, 255, 255);"));
        outputLog = new QTextEdit(frame);
        outputLog->setObjectName("outputLog");
        outputLog->setGeometry(QRect(540, 290, 501, 151));
        outputLog->setFont(font);
        outputLog->setAutoFillBackground(false);
        outputLog->setStyleSheet(QString::fromUtf8("background-color: rgb(0, 0, 0);\n"
"font: 10pt \"Segoe UI\";\n"
"color: rgb(255, 255, 255);"));
        procCount = new QSpinBox(frame);
        procCount->setObjectName("procCount");
        procCount->setGeometry(QRect(20, 390, 81, 41));
        QFont font1;
        font1.setPointSize(15);
        procCount->setFont(font1);
        procCount->setStyleSheet(QString::fromUtf8("/* 1. Style the Main Box */\n"
"QSpinBox {\n"
"    color: white;\n"
"    background-color: #333333;   /* Dark background */\n"
"    border: 2px solid #555555;   /* Gray border */\n"
"    border-radius: 4px;          /* Rounded corners */\n"
"    padding-right: 20px;         /* Make room for the buttons on the right */\n"
"    selection-background-color: #0078d7;\n"
"}\n"
"\n"
"/* 2. Style the UP Button */\n"
"QSpinBox::up-button {\n"
"    subcontrol-origin: border;\n"
"    subcontrol-position: top right; /* Pin to top-right */\n"
"    width: 20px;                    /* Width of the button */\n"
"    \n"
"    background: #555555;            /* Button Color */\n"
"    border-left: 1px solid #333333; /* Separator line */\n"
"    border-bottom: 1px solid #333333;\n"
"    border-top-right-radius: 4px;\n"
"}\n"
"\n"
"/* 3. Style the DOWN Button */\n"
"QSpinBox::down-button {\n"
"    subcontrol-origin: border;\n"
"    subcontrol-position: bottom right; /* Pin to bottom-right */\n"
"    width: 20px;\n"
"    \n"
"    b"
                        "ackground: #555555;\n"
"    border-left: 1px solid #333333;\n"
"    border-top: 1px solid #333333;\n"
"    border-bottom-right-radius: 4px;\n"
"}\n"
"\n"
"/* 4. Add Hover Effects (So you know it's clickable) */\n"
"QSpinBox::up-button:hover, QSpinBox::down-button:hover {\n"
"    background: #777777;\n"
"}\n"
"\n"
"QSpinBox::up-button:pressed, QSpinBox::down-button:pressed {\n"
"    background: #999999;\n"
"}\n"
"\n"
"/* 5. Force Arrows to be visible (Optional, depending on your system) */\n"
"QSpinBox::up-arrow, QSpinBox::down-arrow {\n"
"    width: 7px;\n"
"    height: 7px;\n"
"    background: none; \n"
"    border-left: 1px solid white;  /* Draw a simple arrow using borders */\n"
"    border-top: 1px solid white;   /* (This creates a 'V' shape) */\n"
"}\n"
"\n"
"QSpinBox::up-arrow { \n"
"    image: none; \n"
"    transform: rotate(-45deg);     /* Point up */\n"
"    margin-top: 1px;\n"
"}\n"
"\n"
"QSpinBox::down-arrow { \n"
"    image: none; \n"
"    transform: rotate(135deg);     /* Point down */\n"
"    ma"
                        "rgin-bottom: 1px;\n"
"}"));
        procCount->setMinimum(1);
        procCount->setValue(4);
        label_5 = new QLabel(frame);
        label_5->setObjectName("label_5");
        label_5->setGeometry(QRect(20, 350, 381, 21));
        label_5->setStyleSheet(QString::fromUtf8("color: rgb(255, 255, 255);\n"
"background-color: rgb(43, 45, 48);\n"
"font: 700 12pt \"Segoe UI\";"));
        runButton = new QPushButton(frame);
        runButton->setObjectName("runButton");
        runButton->setGeometry(QRect(20, 10, 93, 29));
        QFont font2;
        font2.setFamilies({QString::fromUtf8("Segoe UI")});
        font2.setBold(true);
        runButton->setFont(font2);
        runButton->setStyleSheet(QString::fromUtf8("QPushButton {\n"
"    /* VS Dark Gray Background */\n"
"    background-color: rgb(43, 45, 48);\n"
"    \n"
"    /* VS Green for the text/icon */\n"
"    color: #5db55b;  \n"
"    \n"
"    /* Subtle border to match the IDE look */\n"
"    border: rgb(43, 45, 48);\n"
"    border-radius: 2px;\n"
"    \n"
"    /* Spacing and Font */\n"
"    padding: 5px 15px;\n"
"    font-family: \"Segoe UI\", sans-serif;\n"
"    font-size: 14px;\n"
"    font-weight: bold;\n"
"}\n"
"\n"
"QPushButton:hover {\n"
"    /* Lighter gray when hovering */\n"
"    background-color: #3e3e40;\n"
"    border-color: #999999;\n"
"}\n"
"\n"
"QPushButton:pressed {\n"
"    /* Darker or Blue-ish accent when clicked */\n"
"    background-color: #007acc;\n"
"    color: white;\n"
"    border-color: #007acc;\n"
"}"));
        label_8 = new QLabel(frame);
        label_8->setObjectName("label_8");
        label_8->setGeometry(QRect(20, 540, 441, 21));
        label_8->setStyleSheet(QString::fromUtf8("color: rgb(255, 255, 255);\n"
"background-color: rgb(43, 45, 48);\n"
"font: 700 12pt \"Segoe UI\";"));
        procCount_3 = new QSpinBox(frame);
        procCount_3->setObjectName("procCount_3");
        procCount_3->setGeometry(QRect(20, 580, 81, 41));
        procCount_3->setFont(font1);
        procCount_3->setStyleSheet(QString::fromUtf8("/* 1. Style the Main Box */\n"
"QSpinBox {\n"
"    color: white;\n"
"    background-color: #333333;   /* Dark background */\n"
"    border: 2px solid #555555;   /* Gray border */\n"
"    border-radius: 4px;          /* Rounded corners */\n"
"    padding-right: 20px;         /* Make room for the buttons on the right */\n"
"    selection-background-color: #0078d7;\n"
"}\n"
"\n"
"/* 2. Style the UP Button */\n"
"QSpinBox::up-button {\n"
"    subcontrol-origin: border;\n"
"    subcontrol-position: top right; /* Pin to top-right */\n"
"    width: 20px;                    /* Width of the button */\n"
"    \n"
"    background: #555555;            /* Button Color */\n"
"    border-left: 1px solid #333333; /* Separator line */\n"
"    border-bottom: 1px solid #333333;\n"
"    border-top-right-radius: 4px;\n"
"}\n"
"\n"
"/* 3. Style the DOWN Button */\n"
"QSpinBox::down-button {\n"
"    subcontrol-origin: border;\n"
"    subcontrol-position: bottom right; /* Pin to bottom-right */\n"
"    width: 20px;\n"
"    \n"
"    b"
                        "ackground: #555555;\n"
"    border-left: 1px solid #333333;\n"
"    border-top: 1px solid #333333;\n"
"    border-bottom-right-radius: 4px;\n"
"}\n"
"\n"
"/* 4. Add Hover Effects (So you know it's clickable) */\n"
"QSpinBox::up-button:hover, QSpinBox::down-button:hover {\n"
"    background: #777777;\n"
"}\n"
"\n"
"QSpinBox::up-button:pressed, QSpinBox::down-button:pressed {\n"
"    background: #999999;\n"
"}\n"
"\n"
"/* 5. Force Arrows to be visible (Optional, depending on your system) */\n"
"QSpinBox::up-arrow, QSpinBox::down-arrow {\n"
"    width: 7px;\n"
"    height: 7px;\n"
"    background: none; \n"
"    border-left: 1px solid white;  /* Draw a simple arrow using borders */\n"
"    border-top: 1px solid white;   /* (This creates a 'V' shape) */\n"
"}\n"
"\n"
"QSpinBox::up-arrow { \n"
"    image: none; \n"
"    transform: rotate(-45deg);     /* Point up */\n"
"    margin-top: 1px;\n"
"}\n"
"\n"
"QSpinBox::down-arrow { \n"
"    image: none; \n"
"    transform: rotate(135deg);     /* Point down */\n"
"    ma"
                        "rgin-bottom: 1px;\n"
"}"));
        procCount_3->setMinimum(1);
        procCount_3->setValue(4);
        outputLog_3 = new QTextEdit(frame);
        outputLog_3->setObjectName("outputLog_3");
        outputLog_3->setGeometry(QRect(540, 520, 501, 151));
        outputLog_3->setFont(font);
        outputLog_3->setAutoFillBackground(false);
        outputLog_3->setStyleSheet(QString::fromUtf8("background-color: rgb(0, 0, 0);\n"
"font: 10pt \"Segoe UI\";\n"
"color: rgb(255, 255, 255);"));
        label_9 = new QLabel(frame);
        label_9->setObjectName("label_9");
        label_9->setGeometry(QRect(540, 480, 371, 21));
        label_9->setStyleSheet(QString::fromUtf8("color: rgb(255, 255, 255);\n"
"background-color: rgb(43, 45, 48);\n"
"font: 700 12pt \"Segoe UI\";"));
        outputLog_4 = new QTextEdit(frame);
        outputLog_4->setObjectName("outputLog_4");
        outputLog_4->setGeometry(QRect(1080, 50, 431, 621));
        outputLog_4->setFont(font);
        outputLog_4->setAutoFillBackground(false);
        outputLog_4->setStyleSheet(QString::fromUtf8("background-color: rgb(0, 0, 0);\n"
"font: 10pt \"Segoe UI\";\n"
"color: rgb(255, 255, 255);"));
        label_10 = new QLabel(frame);
        label_10->setObjectName("label_10");
        label_10->setGeometry(QRect(1080, 10, 211, 21));
        label_10->setStyleSheet(QString::fromUtf8("color: rgb(255, 255, 255);\n"
"background-color: rgb(43, 45, 48);\n"
"font: 700 12pt \"Segoe UI\";"));
        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 1525, 26));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        inputCode->setHtml(QCoreApplication::translate("MainWindow", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><meta charset=\"utf-8\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"hr { height: 1px; border-width: 0; }\n"
"li.unchecked::marker { content: \"\\2610\"; }\n"
"li.checked::marker { content: \"\\2612\"; }\n"
"</style></head><body style=\" font-family:'Segoe UI'; font-size:10pt; font-weight:400; font-style:normal;\">\n"
"<p style=\"-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><br /></p></body></html>", nullptr));
        label_2->setText(QCoreApplication::translate("MainWindow", "Parallel Version", nullptr));
        label_3->setText(QCoreApplication::translate("MainWindow", "Output", nullptr));
        parallelCode->setHtml(QCoreApplication::translate("MainWindow", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><meta charset=\"utf-8\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"hr { height: 1px; border-width: 0; }\n"
"li.unchecked::marker { content: \"\\2610\"; }\n"
"li.checked::marker { content: \"\\2612\"; }\n"
"</style></head><body style=\" font-family:'Segoe UI'; font-size:10pt; font-weight:400; font-style:normal;\">\n"
"<p style=\"-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><br /></p></body></html>", nullptr));
        outputLog->setHtml(QCoreApplication::translate("MainWindow", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><meta charset=\"utf-8\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"hr { height: 1px; border-width: 0; }\n"
"li.unchecked::marker { content: \"\\2610\"; }\n"
"li.checked::marker { content: \"\\2612\"; }\n"
"</style></head><body style=\" font-family:'Segoe UI'; font-size:10pt; font-weight:400; font-style:normal;\">\n"
"<p style=\"-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><br /></p></body></html>", nullptr));
        label_5->setText(QCoreApplication::translate("MainWindow", "Select Number of Processors to run on", nullptr));
        runButton->setText(QCoreApplication::translate("MainWindow", "\342\226\266 Run", nullptr));
        label_8->setText(QCoreApplication::translate("MainWindow", "Enter MAX processors to scale up to (e.g., 8)", nullptr));
        outputLog_3->setHtml(QCoreApplication::translate("MainWindow", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><meta charset=\"utf-8\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"hr { height: 1px; border-width: 0; }\n"
"li.unchecked::marker { content: \"\\2610\"; }\n"
"li.checked::marker { content: \"\\2612\"; }\n"
"</style></head><body style=\" font-family:'Segoe UI'; font-size:10pt; font-weight:400; font-style:normal;\">\n"
"<p style=\"-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><br /></p></body></html>", nullptr));
        label_9->setText(QCoreApplication::translate("MainWindow", "RMSE, Speedup & Efficiency Results", nullptr));
        outputLog_4->setHtml(QCoreApplication::translate("MainWindow", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><meta charset=\"utf-8\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"hr { height: 1px; border-width: 0; }\n"
"li.unchecked::marker { content: \"\\2610\"; }\n"
"li.checked::marker { content: \"\\2612\"; }\n"
"</style></head><body style=\" font-family:'Segoe UI'; font-size:10pt; font-weight:400; font-style:normal;\">\n"
"<p style=\"-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><br /></p></body></html>", nullptr));
        label_10->setText(QCoreApplication::translate("MainWindow", "Analysis/Final_Report", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
