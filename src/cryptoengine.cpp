#include "cryptoengine.h"
#include <QDebug>

CryptoEngine::CryptoEngine()
{
    // 初始化 OpenSSL
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();
}

CryptoEngine::~CryptoEngine()
{
    EVP_cleanup();
    ERR_free_strings();
}

QByteArray CryptoEngine::sha512(const QByteArray &data)
{
    if (data.size() > MAX_PLAINTEXT_SIZE) {
        setError("輸入數據超過大小限制");
        return QByteArray();
    }
    
    unsigned char hash[SHA512_DIGEST_LENGTH];
    SHA512_CTX sha512;
    
    SHA512_Init(&sha512);
    SHA512_Update(&sha512, reinterpret_cast<const unsigned char *>(data.constData()),
                  static_cast<size_t>(data.size()));
    SHA512_Final(hash, &sha512);
    
    return QByteArray(reinterpret_cast<char *>(hash), SHA512_DIGEST_LENGTH);
}

QByteArray CryptoEngine::generateSalt()
{
    unsigned char salt[16];
    
    if (RAND_bytes(salt, sizeof(salt)) != 1) {
        setError("隨機數生成失敗");
        return QByteArray();
    }
    
    return QByteArray(reinterpret_cast<char *>(salt), sizeof(salt));
}

QByteArray CryptoEngine::deriveKey(const SecureString &password,
                                   const QByteArray &salt,
                                   int keyLen,
                                   int iterations)
{
    unsigned char derivedKey[EVP_MAX_KEY_LENGTH];
    
    int result = PKCS5_PBKDF2_HMAC(
        password.constData(),
        password.size(),
        reinterpret_cast<const unsigned char *>(salt.constData()),
        salt.size(),
        iterations,
        EVP_sha256(),
        keyLen,
        derivedKey
    );
    
    if (result != 1) {
        setError("金鑰派生失敗");
        return QByteArray();
    }
    
    return QByteArray(reinterpret_cast<char *>(derivedKey), keyLen);
}

QByteArray CryptoEngine::encryptAES256(const SecureString &plaintext,
                                       const SecureString &password,
                                       const QByteArray &salt)
{
    if (plaintext.size() > MAX_PLAINTEXT_SIZE) {
        setError("明文超過大小限制");
        return QByteArray();
    }
    
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        setError("無法創建加密上下文");
        return QByteArray();
    }
    
    // 派生金鑰和 IV
    QByteArray key = deriveKey(password, salt, 32); // 256 位 = 32 字節
    unsigned char iv[EVP_MAX_IV_LENGTH];
    
    if (RAND_bytes(iv, EVP_MAX_IV_LENGTH) != 1) {
        setError("IV 生成失敗");
        EVP_CIPHER_CTX_free(ctx);
        return QByteArray();
    }
    
    if (key.isEmpty()) {
        EVP_CIPHER_CTX_free(ctx);
        return QByteArray();
    }
    
    // 初始化加密
    int ret = EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr,
                                 reinterpret_cast<const unsigned char *>(key.constData()),
                                 iv);
    
    if (ret != 1) {
        setError("加密初始化失敗");
        EVP_CIPHER_CTX_free(ctx);
        return QByteArray();
    }
    
    // 分配密文緩衝區（含防護）
    QByteArray ciphertext;
    ciphertext.resize(plaintext.size() + EVP_MAX_BLOCK_LENGTH);
    int len = 0;
    int ciphertext_len = 0;
    
    // 加密數據
    ret = EVP_EncryptUpdate(ctx,
                            reinterpret_cast<unsigned char *>(ciphertext.data()),
                            &len,
                            reinterpret_cast<const unsigned char *>(plaintext.constData()),
                            plaintext.size());
    
    if (ret != 1) {
        setError("加密失敗");
        EVP_CIPHER_CTX_free(ctx);
        return QByteArray();
    }
    
    ciphertext_len = len;
    
    // 完成加密
    ret = EVP_EncryptFinal_ex(ctx, reinterpret_cast<unsigned char *>(ciphertext.data() + len),
                              &len);
    
    if (ret != 1) {
        setError("加密完成失敗");
        EVP_CIPHER_CTX_free(ctx);
        return QByteArray();
    }
    
    ciphertext_len += len;
    ciphertext.resize(ciphertext_len);
    
    EVP_CIPHER_CTX_free(ctx);
    
    // 返回格式: IV + 密文
    QByteArray result;
    result.append(reinterpret_cast<const char *>(iv), EVP_MAX_IV_LENGTH);
    result.append(ciphertext);
    
    return result;
}

SecureString CryptoEngine::decryptAES256(const QByteArray &ciphertext,
                                         const SecureString &password,
                                         const QByteArray &salt)
{
    if (ciphertext.size() < EVP_MAX_IV_LENGTH + 1) {
        setError("密文大小不足");
        return SecureString();
    }
    
    if (ciphertext.size() > MAX_CIPHERTEXT_SIZE) {
        setError("密文超過大小限制");
        return SecureString();
    }
    
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        setError("無法創建解密上下文");
        return SecureString();
    }
    
    // 派生金鑰
    QByteArray key = deriveKey(password, salt, 32);
    if (key.isEmpty()) {
        EVP_CIPHER_CTX_free(ctx);
        return SecureString();
    }
    
    // 提取 IV
    const unsigned char *iv = reinterpret_cast<const unsigned char *>(ciphertext.constData());
    const unsigned char *encrypted_data = iv + EVP_MAX_IV_LENGTH;
    int encrypted_len = ciphertext.size() - EVP_MAX_IV_LENGTH;
    
    // 初始化解密
    int ret = EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr,
                                 reinterpret_cast<const unsigned char *>(key.constData()),
                                 iv);
    
    if (ret != 1) {
        setError("解密初始化失敗");
        EVP_CIPHER_CTX_free(ctx);
        return SecureString();
    }
    
    // 分配明文緩衝區
    SecureString plaintext(QByteArray(encrypted_len + EVP_MAX_BLOCK_LENGTH, 0));
    int len = 0;
    int plaintext_len = 0;
    
    // 解密數據
    ret = EVP_DecryptUpdate(ctx,
                            reinterpret_cast<unsigned char *>(plaintext.data()),
                            &len,
                            encrypted_data,
                            encrypted_len);
    
    if (ret != 1) {
        setError("解密失敗");
        EVP_CIPHER_CTX_free(ctx);
        return SecureString();
    }
    
    plaintext_len = len;
    
    // 完成解密
    ret = EVP_DecryptFinal_ex(ctx, reinterpret_cast<unsigned char *>(plaintext.data() + len),
                              &len);
    
    if (ret != 1) {
        setError("解密完成失敗");
        EVP_CIPHER_CTX_free(ctx);
        return SecureString();
    }
    
    plaintext_len += len;
    
    EVP_CIPHER_CTX_free(ctx);
    
    // 調整大小
    SecureString result(plaintext.toByteArray().left(plaintext_len));
    return result;
}

QByteArray CryptoEngine::encryptRSA4096(const QByteArray &plaintext,
                                        const QByteArray &publicKeyPem)
{
    if (plaintext.size() > MAX_PLAINTEXT_SIZE) {
        setError("明文超過大小限制");
        return QByteArray();
    }
    
    BIO *bio = BIO_new_mem_buf(publicKeyPem.constData(), publicKeyPem.size());
    if (!bio) {
        setError("無法創建 BIO");
        return QByteArray();
    }
    
    RSA *rsa = PEM_read_bio_RSAPublicKey(bio, nullptr, nullptr, nullptr);
    if (!rsa) {
        // 嘗試讀取 PKCS#8 格式
        BIO_reset(bio);
        EVP_PKEY *pkey = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
        BIO_free(bio);
        
        if (!pkey) {
            setError("無法讀取公鑰");
            return QByteArray();
        }
        
        rsa = EVP_PKEY_get1_RSA(pkey);
        EVP_PKEY_free(pkey);
        
        if (!rsa) {
            setError("無法提取 RSA 公鑰");
            return QByteArray();
        }
    } else {
        BIO_free(bio);
    }
    
    // 分配密文緩衝區
    int rsa_size = RSA_size(rsa);
    QByteArray ciphertext(rsa_size, 0);
    
    // 執行加密
    int ret = RSA_public_encrypt(
        plaintext.size(),
        reinterpret_cast<const unsigned char *>(plaintext.constData()),
        reinterpret_cast<unsigned char *>(ciphertext.data()),
        rsa,
        RSA_PKCS1_OAEP_PADDING
    );
    
    RSA_free(rsa);
    
    if (ret == -1) {
        setError("RSA 加密失敗");
        return QByteArray();
    }
    
    ciphertext.resize(ret);
    return ciphertext;
}

SecureString CryptoEngine::decryptRSA4096(const QByteArray &ciphertext,
                                          const SecureString &privateKeyPem)
{
    if (ciphertext.size() > MAX_CIPHERTEXT_SIZE) {
        setError("密文超過大小限制");
        return SecureString();
    }
    
    BIO *bio = BIO_new_mem_buf(privateKeyPem.constData(), privateKeyPem.size());
    if (!bio) {
        setError("無法創建 BIO");
        return SecureString();
    }
    
    RSA *rsa = PEM_read_bio_RSAPrivateKey(bio, nullptr, nullptr, nullptr);
    if (!rsa) {
        // 嘗試讀取 PKCS#8 格式
        BIO_reset(bio);
        EVP_PKEY *pkey = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr);
        BIO_free(bio);
        
        if (!pkey) {
            setError("無法讀取私鑰");
            return SecureString();
        }
        
        rsa = EVP_PKEY_get1_RSA(pkey);
        EVP_PKEY_free(pkey);
        
        if (!rsa) {
            setError("無法提取 RSA 私鑰");
            return SecureString();
        }
    } else {
        BIO_free(bio);
    }
    
    int rsa_size = RSA_size(rsa);
    SecureString plaintext(QByteArray(rsa_size, 0));
    
    // 執行解密
    int ret = RSA_private_decrypt(
        ciphertext.size(),
        reinterpret_cast<const unsigned char *>(ciphertext.constData()),
        reinterpret_cast<unsigned char *>(plaintext.data()),
        rsa,
        RSA_PKCS1_OAEP_PADDING
    );
    
    RSA_free(rsa);
    
    if (ret == -1) {
        setError("RSA 解密失敗");
        return SecureString();
    }
    
    SecureString result(plaintext.toByteArray().left(ret));
    return result;
}

bool CryptoEngine::generateRSA4096KeyPair(QByteArray &publicKey, SecureString &privateKey)
{
    RSA *rsa = RSA_new();
    BIGNUM *bne = BN_new();
    
    if (!rsa || !bne) {
        setError("無法分配內存");
        RSA_free(rsa);
        BN_free(bne);
        return false;
    }
    
    // 設置公開指數
    if (BN_set_word(bne, RSA_F4) != 1) {
        setError("設置公開指數失敗");
        RSA_free(rsa);
        BN_free(bne);
        return false;
    }
    
    // 生成 4096 位金鑰
    if (RSA_generate_key_ex(rsa, 4096, bne, nullptr) != 1) {
        setError("RSA 金鑰生成失敗");
        RSA_free(rsa);
        BN_free(bne);
        return false;
    }
    
    BN_free(bne);
    
    // 寫入公鑰
    BIO *bio_pub = BIO_new(BIO_s_mem());
    if (PEM_write_bio_RSAPublicKey(bio_pub, rsa) != 1) {
        setError("公鑰寫入失敗");
        BIO_free(bio_pub);
        RSA_free(rsa);
        return false;
    }
    
    BUF_MEM *buffer_pub;
    BIO_get_mem_ptr(bio_pub, &buffer_pub);
    publicKey = QByteArray(buffer_pub->data, buffer_pub->length);
    BIO_free(bio_pub);
    
    // 寫入私鑰
    BIO *bio_priv = BIO_new(BIO_s_mem());
    if (PEM_write_bio_RSAPrivateKey(bio_priv, rsa, nullptr, nullptr, 0, nullptr, nullptr) != 1) {
        setError("私鑰寫入失敗");
        BIO_free(bio_priv);
        RSA_free(rsa);
        return false;
    }
    
    BUF_MEM *buffer_priv;
    BIO_get_mem_ptr(bio_priv, &buffer_priv);
    privateKey = SecureString(QByteArray(buffer_priv->data, buffer_priv->length));
    BIO_free(bio_priv);
    
    RSA_free(rsa);
    return true;
}
