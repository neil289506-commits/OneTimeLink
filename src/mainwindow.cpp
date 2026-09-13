#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QMessageBox>
#include <QClipboard>
#include <QApplication>
#include <QFileDialog>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_keysConfigured(false),
      m_generator(std::make_unique<OTLGenerator>())
{
    setWindowTitle("OTL 一次性連結生成器");
    setGeometry(100, 100, 900, 700);
    
    setupUI();
    setupConnections();
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUI()
{
    // 中心 Widget
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    // ===== 標題 =====
    QLabel *titleLabel = new QLabel("OTL（One-Time Link）生成器");
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #333;");
    mainLayout->addWidget(titleLabel);
    
    // ===== Cloudflare 配置群組 =====
    QGroupBox *configGroup = new QGroupBox("Cloudflare 配置", this);
    QVBoxLayout *configLayout = new QVBoxLayout(configGroup);
    
    QLabel *cloudflareLabel = new QLabel("Cloudflare Workers 基礎 URL:");
    m_cloudflareUrlInput = new QLineEdit(this);
    m_cloudflareUrlInput->setPlaceholderText("例如: https://otl.example.com");
    m_cloudflareUrlInput->setText("https://otl.example.com");
    configLayout->addWidget(cloudflareLabel);
    configLayout->addWidget(m_cloudflareUrlInput);
    
    mainLayout->addWidget(configGroup);
    
    // ===== 密鑰配置群組 =====
    QGroupBox *keyGroup = new QGroupBox("密鑰配置", this);
    QHBoxLayout *keyLayout = new QHBoxLayout(keyGroup);
    
    m_setupKeysButton = new QPushButton("配置 RSA-4096 公鑰", this);
    m_setupKeysButton->setMaximumWidth(200);
    
    QLabel *keyStatusLabel = new QLabel("狀態: 未配置");
    keyStatusLabel->setObjectName("keyStatusLabel");
    keyStatusLabel->setStyleSheet("color: #d32f2f;");
    
    keyLayout->addWidget(m_setupKeysButton);
    keyLayout->addWidget(keyStatusLabel);
    keyLayout->addStretch();
    
    mainLayout->addWidget(keyGroup);
    
    // ===== URL 輸入群組 =====
    QGroupBox *inputGroup = new QGroupBox("輸入URL", this);
    QVBoxLayout *inputLayout = new QVBoxLayout(inputGroup);
    
    QLabel *urlLabel = new QLabel("目標 URL:");
    m_urlInput = new QLineEdit(this);
    m_urlInput->setPlaceholderText("輸入你想要保護的URL...");
    
    inputLayout->addWidget(urlLabel);
    inputLayout->addWidget(m_urlInput);
    
    mainLayout->addWidget(inputGroup);
    
    // ===== 操作按鈕 =====
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    m_generateButton = new QPushButton("生成一次性連結", this);
    m_generateButton->setMaximumWidth(200);
    m_generateButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #1976d2;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 4px;"
        "    padding: 8px 16px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #1565c0;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #0d47a1;"
        "}"
        "QPushButton:disabled {"
        "    background-color: #ccc;"
        "    color: #999;"
        "}"
    );
    
    m_progressBar = new QProgressBar(this);
    m_progressBar->setMaximumHeight(20);
    m_progressBar->setVisible(false);
    
    buttonLayout->addWidget(m_generateButton);
    buttonLayout->addWidget(m_progressBar);
    buttonLayout->addStretch();
    
    mainLayout->addLayout(buttonLayout);
    
    // ===== 結果群組 =====
    QGroupBox *resultGroup = new QGroupBox("生成結果", this);
    QVBoxLayout *resultLayout = new QVBoxLayout(resultGroup);
    
    m_resultDisplay = new QTextEdit(this);
    m_resultDisplay->setReadOnly(true);
    m_resultDisplay->setMinimumHeight(200);
    m_resultDisplay->setStyleSheet(
        "QTextEdit {"
        "    background-color: #f5f5f5;"
        "    border: 1px solid #ddd;"
        "    border-radius: 4px;"
        "    padding: 8px;"
        "    font-family: 'Courier New';"
        "    font-size: 10px;"
        "}"
    );
    
    resultLayout->addWidget(m_resultDisplay);
    
    m_copyLinkButton = new QPushButton("複製連結", this);
    m_copyLinkButton->setMaximumWidth(150);
    m_copyLinkButton->setEnabled(false);
    
    resultLayout->addWidget(m_copyLinkButton);
    
    mainLayout->addWidget(resultGroup);
    
    // ===== 狀態欄 =====
    m_statusLabel = new QLabel("就緒", this);
    mainLayout->addWidget(m_statusLabel);
    
    mainLayout->addStretch();
}

void MainWindow::setupConnections()
{
    connect(m_generateButton, &QPushButton::clicked, this, &MainWindow::onGenerateButtonClicked);
    connect(m_copyLinkButton, &QPushButton::clicked, this, &MainWindow::onCopyLinkClicked);
    connect(m_setupKeysButton, &QPushButton::clicked, this, &MainWindow::onSetupKeysClicked);
    connect(m_urlInput, &QLineEdit::textChanged, this, &MainWindow::onUrlTextChanged);
    connect(m_cloudflareUrlInput, &QLineEdit::textChanged, this, &MainWindow::onCloudflareUrlChanged);
}

void MainWindow::onSetupKeysClicked()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "選擇 RSA 公鑰文件",
        "",
        "PEM Files (*.pem);;All Files (*)"
    );
    
    if (fileName.isEmpty()) {
        return;
    }
    
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        showErrorMessage("錯誤", "無法打開文件: " + fileName);
        return;
    }
    
    m_publicKey = file.readAll();
    file.close();
    
    if (m_publicKey.isEmpty()) {
        showErrorMessage("錯誤", "文件為空");
        return;
    }
    
    m_generator->setPublicKey(m_publicKey);
    m_keysConfigured = true;
    
    findChild<QLabel *>("keyStatusLabel")->setText("狀態: 已配置 ✓");
    findChild<QLabel *>("keyStatusLabel")->setStyleSheet("color: #388e3c;");
    
    showSuccessMessage("成功", "RSA-4096 公鑰已加載");
}

void MainWindow::onGenerateButtonClicked()
{
    QString url = m_urlInput->text().trimmed();
    QString cloudflareUrl = m_cloudflareUrlInput->text().trimmed();
    
    if (url.isEmpty()) {
        showErrorMessage("驗證錯誤", "請輸入目標 URL");
        return;
    }
    
    if (cloudflareUrl.isEmpty()) {
        showErrorMessage("驗證錯誤", "請輸入 Cloudflare 基礎 URL");
        return;
    }
    
    m_generateButton->setEnabled(false);
    m_progressBar->setVisible(true);
    m_statusLabel->setText("正在生成一次性連結...");
    
    // 模擬進度
    m_progressBar->setValue(0);
    QApplication::processEvents();
    
    try {
        m_generator->setCloudflareBaseUrl(cloudflareUrl);
        
        auto result = m_generator->generateLink(url);
        
        m_progressBar->setValue(100);
        
        if (result.error.isEmpty()) {
            displayResult(result);
            m_statusLabel->setText("一次性連結已生成");
        } else {
            showErrorMessage("生成失敗", result.error);
            m_statusLabel->setText("生成失敗");
        }
    } catch (const std::exception &e) {
        showErrorMessage("異常", QString::fromStdString(e.what()));
        m_statusLabel->setText("發生異常");
    }
    
    m_progressBar->setVisible(false);
    m_generateButton->setEnabled(true);
}

void MainWindow::onCopyLinkClicked()
{
    if (m_lastGeneratedLink.isEmpty()) {
        showErrorMessage("錯誤", "沒有可複製的連結");
        return;
    }
    
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(m_lastGeneratedLink);
    
    showSuccessMessage("成功", "一次性連結已複製到剪貼板");
}

void MainWindow::onUrlTextChanged()
{
    // 可在此進行實時驗證
}

void MainWindow::onCloudflareUrlChanged()
{
    // 可在此進行實時驗證
}

void MainWindow::displayResult(const OTLGenerator::GeneratedLink &result)
{
    m_lastGeneratedLink = result.oneTimeLink;
    
    QString display = QString(
        "=== 生成結果 ===\n\n"
        "目標 URL:\n%1\n\n"
        "一次性連結:\n%2\n\n"
        "令牌 (Token):\n%3\n\n"
        "鹽 (Salt) [Base64]:\n%4\n\n"
        "加密 URL [Base64]:\n%5\n\n"
        "加密令牌 [Base64]:\n%6\n"
    ).arg(
        m_urlInput->text(),
        result.oneTimeLink,
        result.token,
        QString::fromLatin1(result.salt.toBase64()),
        QString::fromLatin1(result.encryptedUrl.toBase64()),
        QString::fromLatin1(result.encryptedToken.toBase64())
    );
    
    m_resultDisplay->setPlainText(display);
    m_copyLinkButton->setEnabled(true);
}

void MainWindow::showErrorMessage(const QString &title, const QString &message)
{
    QMessageBox::critical(this, title, message);
}

void MainWindow::showSuccessMessage(const QString &title, const QString &message)
{
    QMessageBox::information(this, title, message);
}
