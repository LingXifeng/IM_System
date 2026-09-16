#ifndef CRYPTOMANAGER_H
#define CRYPTOMANAGER_H

#include <QByteArray>

    class CryptoManager
{
public:
    // 使用 AES-256-GCM 加密
    //
    // plaintext:
    //     要加密的明文
    //
    // key:
    //     32 字节 AES-256 密钥
    //
    // nonce:
    //     输出的随机 nonce
    //
    // tag:
    //     输出的 GCM authentication tag
    //
    // 返回：
    //     ciphertext
    static QByteArray encrypt(
        const QByteArray& plaintext,
        const QByteArray& key,
        QByteArray& nonce,
        QByteArray& tag
        );

    // 使用 AES-256-GCM 解密
    //
    // ciphertext:
    //     密文
    //
    // key:
    //     32 字节 AES-256 密钥
    //
    // nonce:
    //     加密时生成的 nonce
    //
    // tag:
    //     加密时生成的 authentication tag
    //
    // 返回：
    //     plaintext
    //
    // 如果认证失败，返回空 QByteArray。
    static QByteArray decrypt(
        const QByteArray& ciphertext,
        const QByteArray& key,
        const QByteArray& nonce,
        const QByteArray& tag
        );
};

#endif // CRYPTOMANAGER_H