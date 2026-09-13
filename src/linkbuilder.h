#ifndef LINKBUILDER_H
#define LINKBUILDER_H

#include <QString>
#include <QByteArray>

/**
 * @class LinkBuilder
 * @brief 一次性連結構建器
 * 
 * 連結格式：
 * https://<cloudflare網站>!redirect=<Base64URL(加密URL)>&salt=<Base64URL(鹽)>?x=<Base64URL(加密令牌)>
 */
class LinkBuilder
{
public:
    LinkBuilder() = default;
    
    /**
     * 構建一次性連結
     * 
     * @param baseUrl Cloudflare 基礎 URL
     * @param encryptedUrl AES256 加密的 URL
     * @param encryptedToken RSA4096 加密的令牌
     * @param salt 鹽值
     * @return 完整的一次性連結
     */
    QString buildLink(const QString &baseUrl,
                     const QByteArray &encryptedUrl,
                     const QByteArray &encryptedToken,
                     const QByteArray &salt);
    
    /**
     * 解析一次性連結
     * 
     * @param link 完整的一次性連結
     * @param encryptedUrl 輸出：加密的 URL
     * @param encryptedToken 輸出：加密的令牌
     * @param salt 輸出：鹽值
     * @return 成功返回 true
     */
    static bool parseLink(const QString &link,
                         QByteArray &encryptedUrl,
                         QByteArray &encryptedToken,
                         QByteArray &salt);
    
    /**
     * Base64URL 解碼
     */
    static QByteArray base64UrlDecode(const QString &data);

private:
    static QString base64UrlEncode(const QByteArray &data);
};

#endif // LINKBUILDER_H
