#include "otlgenerator.h"
#include <QUrl>
#include <QUuid>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>

OTLGenerator::OTLGenerator(const QString &cloudflareBaseUrl)
    : m_cloudflareBaseUrl(cloudflareBaseUrl),
      m_cryptoEngine(std::make_unique<CryptoEngine>()),
      m_linkBuilder(std::make_unique<LinkBuilder>())
{
}

OTLGenerator::~OTLGenerator() = default;

QString OTLGenerator::generateRandomToken()
{
    // 使用 UUID 作為令牌基礎，確保唯一性
    return QUuid::createUuid().toString().remove("{").remove("}");
}

QString OTLGenerator::base64UrlEncode(const QByteArray &data)
{
    // 標準 Base64 編碼
    QString encoded = QString::fromLatin1(data.toBase64());
    
    // 轉換為 Base64URL（RFC 4648）
    encoded.replace('+', '-').replace('/', '_');
    
    // 移除填充
    while (encoded.endsWith('=')) {
        encoded.chop(1);
    }
    
    return encoded;
}

OTLGenerator::GeneratedLink OTLGenerator::generateLink(const QString &targetUrl)
{
    QString token = generateRandomToken();
    return generateLinkWithToken(targetUrl, token);
}

OTLGenerator::GeneratedLink OTLGenerator::generateLinkWithToken(const QString &targetUrl,
                                                                const QString &token)
{
    GeneratedLink result;
    result.token = token;
    
    // 驗證 URL
    QUrl url(targetUrl);
    if (!url.isValid()) {
        result.error = "無效的 URL";
        m_lastError = result.error;
        return result;
    }
    
    // 生成隨機鹽
    result.salt = m_cryptoEngine->generateSalt();
    if (result.salt.isEmpty()) {
        result.error = "鹽生成失敗: " + m_cryptoEngine->lastError();
        m_lastError = result.error;
        return result;
    }
    
    // 步驟 1: SHA512 雜湊 URL + 鹽
    QByteArray urlWithSalt = targetUrl.toUtf8() + result.salt;
    QByteArray urlHash = m_cryptoEngine->sha512(urlWithSalt);
    
    if (urlHash.isEmpty()) {
        result.error = "SHA512 雜湊失敗";
        m_lastError = result.error;
        return result;
    }
    
    // 步驟 2: AES256 加密雜湊結果
    SecureString password(m_password.isEmpty() ? 
                         SecureString(QByteArray("default_password_change_this")) :
                         m_password);
    
    result.encryptedUrl = m_cryptoEngine->encryptAES256(
        SecureString(urlHash),
        password,
        result.salt
    );
    
    if (result.encryptedUrl.isEmpty()) {
        result.error = "AES256 加密失敗: " + m_cryptoEngine->lastError();
        m_lastError = result.error;
        return result;
    }
    
    // 步驟 3: RSA4096 加密令牌
    if (!m_publicKey.isEmpty()) {
        result.encryptedToken = m_cryptoEngine->encryptRSA4096(
            token.toUtf8(),
            m_publicKey
        );
        
        if (result.encryptedToken.isEmpty()) {
            result.error = "RSA4096 加密失敗: " + m_cryptoEngine->lastError();
            m_lastError = result.error;
            return result;
        }
    } else {
        // 如果沒有公鑰，使用原始令牌
        result.encryptedToken = token.toUtf8();
    }
    
    // 步驟 4: 構建完整連結 (本地加密格式保留)
    result.oneTimeLink = m_linkBuilder->buildLink(
        m_cloudflareBaseUrl,
        result.encryptedUrl,
        result.encryptedToken,
        result.salt
    );
    
    if (result.oneTimeLink.isEmpty()) {
        result.error = "連結構建失敗";
        m_lastError = result.error;
        return result;
    }
    
    // 步驟 5: 請求 Cloudflare Workers API 註冊 Token 並獲取最終跳轉連結
    QString apiUrl = m_cloudflareBaseUrl;
    if (apiUrl.endsWith('/')) {
        apiUrl.chop(1);
    }
    apiUrl += "/api/tokens/create";
    
    QNetworkRequest request((QUrl(apiUrl)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QJsonObject jsonObj;
    jsonObj["targetUrl"] = targetUrl;
    jsonObj["expirationSeconds"] = 3600;
    jsonObj["token"] = token; // 將 UUID 同步給後端
    
    QJsonDocument doc(jsonObj);
    QByteArray postData = doc.toJson();
    
    QEventLoop loop;
    QNetworkReply *reply = m_networkManager.post(request, postData);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray responseData = reply->readAll();
        QJsonDocument responseDoc = QJsonDocument::fromJson(responseData);
        if (responseDoc.isObject() && responseDoc.object()["success"].toBool()) {
            // 成功註冊後，將回傳的 Cloudflare API 網址覆蓋成本次產生的連結
            result.oneTimeLink = responseDoc.object()["redirectUrl"].toString();
        } else {
            qWarning() << "API 回應錯誤，退回使用本地端生成的連結格式";
        }
    } else {
        qWarning() << "網路請求失敗: " << reply->errorString() << "，退回使用本地端生成的連結格式";
    }
    reply->deleteLater();
    
    return result;
}