#ifndef CRYPTOENGINE_H
#define CRYPTOENGINE_H

#include <QByteArray>
#include <QString>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include "securestring.h"

/**
 * @class CryptoEngine
 * @brief 密碼學引擎 - SHA512, AES-256-CBC, RSA-4096
 * 
 * 功能：
 * - SHA512 雜湊（用於鹽化網址）
 * - AES-256-CBC 加密/解密
 * - RSA-4096 加密/解密（令牌加密）
 * - 防禦緩衝區溢出
 * - 安全的隨機數生成
 */
class CryptoEngine
{
public:
    CryptoEngine();
    ~CryptoEngine();
    
    // SHA512 雜湊
    QByteArray sha512(const QByteArray &data);
    
    // 生成隨機鹽（16 字節）
    QByteArray generateSalt();
    
    // AES-256-CBC 加密
    // 返回格式: IV(16字節) + 密文
    QByteArray encryptAES256(const SecureString &plaintext,
                             const SecureString &password,
                             const QByteArray &salt);
    
    // AES-256-CBC 解密
    SecureString decryptAES256(const QByteArray &ciphertext,
                               const SecureString &password,
                               const QByteArray &salt);
    
    // RSA-4096 公鑰加密
    QByteArray encryptRSA4096(const QByteArray &plaintext,
                              const QByteArray &publicKey);
    
    // RSA-4096 私鑰解密
    SecureString decryptRSA4096(const QByteArray &ciphertext,
                                const SecureString &privateKey);
    
    // 生成 RSA-4096 金鑰對
    bool generateRSA4096KeyPair(QByteArray &publicKey, SecureString &privateKey);
    
    // 獲取最後的錯誤信息
    QString lastError() const { return m_lastError; }
    
private:
    // PBKDF2 派生金鑰
    QByteArray deriveKey(const SecureString &password,
                         const QByteArray &salt,
                         int keyLen,
                         int iterations = 100000);
    
    void setError(const QString &error) { m_lastError = error; }
    QString m_lastError;
    
    // 防禦緩衝區溢出
    static constexpr size_t MAX_PLAINTEXT_SIZE = 1024 * 1024; // 1MB 限制
    static constexpr size_t MAX_CIPHERTEXT_SIZE = 1024 * 1024 + 1024; // 緩衝區
};

#endif // CRYPTOENGINE_H
