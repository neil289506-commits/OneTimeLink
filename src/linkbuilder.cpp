#include "linkbuilder.h"
#include <QUrl>
#include <QUrlQuery>
#include <QDebug>

QString LinkBuilder::base64UrlEncode(const QByteArray &data)
{
    QString encoded = QString::fromLatin1(data.toBase64());
    encoded.replace('+', '-').replace('/', '_');
    while (encoded.endsWith('=')) {
        encoded.chop(1);
    }
    return encoded;
}

QByteArray LinkBuilder::base64UrlDecode(const QString &data)
{
    QString padded = data;
    
    // 添加填充
    switch (padded.length() % 4) {
        case 2: padded += "=="; break;
        case 3: padded += "=";  break;
    }
    
    // 轉換回標準 Base64
    padded.replace('-', '+').replace('_', '/');
    
    return QByteArray::fromBase64(padded.toLatin1());
}

QString LinkBuilder::buildLink(const QString &baseUrl,
                              const QByteArray &encryptedUrl,
                              const QByteArray &encryptedToken,
                              const QByteArray &salt)
{
    // 檢查輸入
    if (baseUrl.isEmpty() || encryptedUrl.isEmpty() || encryptedToken.isEmpty() || salt.isEmpty()) {
        qWarning() << "無效的輸入參數";
        return QString();
    }
    
    // Base64URL 編碼
    QString encodedUrl = base64UrlEncode(encryptedUrl);
    QString encodedToken = base64UrlEncode(encryptedToken);
    QString encodedSalt = base64UrlEncode(salt);
    
    // 構建連結
    // 格式: https://example.com!redirect=<encodedUrl>&salt=<encodedSalt>?x=<encodedToken>
    QString link = baseUrl;
    
    // 確保基礎 URL 不以 / 結尾
    if (link.endsWith('/')) {
        link.chop(1);
    }
    
    // 添加 fragment（!redirect 是 fragment 的一部分）
    link += QString("#redirect=%1&salt=%2?x=%3")
        .arg(encodedUrl, encodedSalt, encodedToken);
    
    return link;
}

bool LinkBuilder::parseLink(const QString &link,
                           QByteArray &encryptedUrl,
                           QByteArray &encryptedToken,
                           QByteArray &salt)
{
    QUrl url(link);
    
    // 提取 fragment
    QString fragment = url.fragment();
    
    // Fragment 格式: redirect=<encodedUrl>&salt=<encodedSalt>
    // Query 格式: x=<encodedToken>
    
    // 解析 fragment
    QUrlQuery fragmentQuery(fragment);
    QString encodedUrl = fragmentQuery.queryItemValue("redirect");
    QString encodedSalt = fragmentQuery.queryItemValue("salt");
    
    // 解析 query
    QUrlQuery queryParams(url.query());
    QString encodedToken = queryParams.queryItemValue("x");
    
    // 檢查必要參數
    if (encodedUrl.isEmpty() || encodedToken.isEmpty() || encodedSalt.isEmpty()) {
        qWarning() << "缺少必要的連結參數";
        return false;
    }
    
    // 解碼
    encryptedUrl = base64UrlDecode(encodedUrl);
    encryptedToken = base64UrlDecode(encodedToken);
    salt = base64UrlDecode(encodedSalt);
    
    // 驗證解碼結果
    if (encryptedUrl.isEmpty() || encryptedToken.isEmpty() || salt.isEmpty()) {
        qWarning() << "Base64URL 解碼失敗";
        return false;
    }
    
    return true;
}
