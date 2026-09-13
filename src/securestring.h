#ifndef SECURESTRING_H
#define SECURESTRING_H

#include <QString>
#include <QByteArray>
#include <cstring>

/**
 * @class SecureString
 * @brief 安全字符串容器，防止內存洩漏和緩衝區溢出
 * 
 * 特點：
 * - 自動清零敏感數據
 * - 防禦緩衝區溢出
 * - 安全的複製和移動語義
 */
class SecureString
{
public:
    SecureString() = default;
    
    explicit SecureString(const QString &str)
        : m_data(str.toLatin1())
    {
        if (m_data.size() > MAX_SIZE) {
            m_data.truncate(MAX_SIZE);
        }
    }
    
    explicit SecureString(const QByteArray &arr)
        : m_data(arr.left(MAX_SIZE))
    {
    }
    
    ~SecureString()
    {
        secureErase();
    }
    
    SecureString(const SecureString &other)
        : m_data(other.m_data.left(MAX_SIZE))
    {
    }
    
    SecureString &operator=(const SecureString &other)
    {
        if (this != &other) {
            secureErase();
            m_data = other.m_data.left(MAX_SIZE);
        }
        return *this;
    }
    
    SecureString(SecureString &&other) noexcept
        : m_data(std::move(other.m_data))
    {
    }
    
    SecureString &operator=(SecureString &&other) noexcept
    {
        if (this != &other) {
            secureErase();
            m_data = std::move(other.m_data);
        }
        return *this;
    }
    
    QByteArray toByteArray() const { return m_data; }
    QString toString() const { return QString::fromLatin1(m_data); }
    
    int size() const { return m_data.size(); }
    bool isEmpty() const { return m_data.isEmpty(); }
    
    const char *constData() const { return m_data.constData(); }
    char *data() { return m_data.data(); }
    
    void clear()
    {
        secureErase();
        m_data.clear();
    }
    
private:
    static constexpr int MAX_SIZE = 4096; // 最大 4KB
    
    QByteArray m_data;
    
    void secureErase()
    {
        // 使用 volatile 防止編譯器優化
        volatile char *ptr = reinterpret_cast<volatile char *>(m_data.data());
        for (int i = 0; i < m_data.size(); ++i) {
            ptr[i] = 0;
        }
    }
};

#endif // SECURESTRING_H
