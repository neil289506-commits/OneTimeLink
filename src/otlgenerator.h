#ifndef OTLGENERATOR_H
#define OTLGENERATOR_H

#include "cryptoengine.h"
#include "linkbuilder.h"
#include <QString>
#include <QByteArray>
#include <QNetworkAccessManager>
#include <memory>

/**
 * @class OTLGenerator
 * @brief OTL（One-Time Link）生成器核心邏輯
 * 
 * 流程：
 * 1. 輸入網址（URL）
 * 2. 生成隨機令牌（Token）
 * 3. 使用 SHA512 + 鹽進行雜湊處理網址
 * 4. 使用 AES256 加密雜湊結果
 * 5. 使用 RSA4096 加密令牌
 * 6. 構建完整的一次性連結
 * 7. 上傳至 Cloudflare 後端
 */
class OTLGenerator
{
public:
    struct GeneratedLink
    {
        QString oneTimeLink;      // 完整的一次性連結
        QString token;            // 未加密的令牌（用於後端存儲）
        QByteArray encryptedUrl;  // 加密的 URL
        QByteArray encryptedToken; // 加密的令牌
        QByteArray salt;          // 使用的鹽
        QString error;            // 錯誤信息
    };
    
    OTLGenerator(const QString &cloudflareBaseUrl = "https://otl.example.com");
    ~OTLGenerator();
    
    // 生成一次性連結
    GeneratedLink generateLink(const QString &targetUrl);
    
    // 生成一次性連結（使用自定義令牌）
    GeneratedLink generateLinkWithToken(const QString &targetUrl, const QString &token);
    
    // 設置 Cloudflare 基礎 URL
    void setCloudflareBaseUrl(const QString &url) { m_cloudflareBaseUrl = url; }
    
    // 設置公鑰（用於 RSA 加密）
    void setPublicKey(const QByteArray &publicKey) { m_publicKey = publicKey; }
    
    // 設置密碼（用於 AES256 派生金鑰）
    void setPassword(const SecureString &password) { m_password = password; }
    
    // 獲取最後的錯誤
    QString lastError() const { return m_lastError; }
    
private:
    std::unique_ptr<CryptoEngine> m_cryptoEngine;
    std::unique_ptr<LinkBuilder> m_linkBuilder;
    QNetworkAccessManager m_networkManager;
    
    QString m_cloudflareBaseUrl;
    QByteArray m_publicKey;
    SecureString m_password;
    
    QString m_lastError;
    
    // 生成隨機令牌（UUID 格式）
    QString generateRandomToken();
    
    // Base64URL 編碼
    static QString base64UrlEncode(const QByteArray &data);
};

#endif // OTLGENERATOR_H