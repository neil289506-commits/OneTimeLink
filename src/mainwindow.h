#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QProgressBar>
#include <memory>
#include "otlgenerator.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onGenerateButtonClicked();
    void onCopyLinkClicked();
    void onSetupKeysClicked();
    void onUrlTextChanged();
    void onCloudflareUrlChanged();

private:
    void setupUI();
    void setupConnections();
    void displayResult(const OTLGenerator::GeneratedLink &result);
    void showErrorMessage(const QString &title, const QString &message);
    void showSuccessMessage(const QString &title, const QString &message);
    
    // UI 組件
    QLineEdit *m_urlInput;
    QLineEdit *m_cloudflareUrlInput;
    QPushButton *m_generateButton;
    QPushButton *m_copyLinkButton;
    QPushButton *m_setupKeysButton;
    QTextEdit *m_resultDisplay;
    QLabel *m_statusLabel;
    QProgressBar *m_progressBar;
    
    // 邏輯
    std::unique_ptr<OTLGenerator> m_generator;
    QString m_lastGeneratedLink;
    
    // 密鑰管理
    QByteArray m_publicKey;
    bool m_keysConfigured;
};

#endif // MAINWINDOW_H
