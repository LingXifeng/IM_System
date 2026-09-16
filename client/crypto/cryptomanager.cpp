#include "CryptoManager.h"

#include <openssl/evp.h>
#include <openssl/rand.h>

    namespace
{
    constexpr int AES_KEY_SIZE = 32;   // AES-256
    constexpr int GCM_NONCE_SIZE = 12;
    constexpr int GCM_TAG_SIZE = 16;
}

QByteArray CryptoManager::encrypt(
    const QByteArray& plaintext,
    const QByteArray& key,
    QByteArray& nonce,
    QByteArray& tag)
{
    if (key.size() != AES_KEY_SIZE)
    {
        return {};
    }

    /*
     * GCM 推荐使用 12 字节 nonce。
     */
    nonce.resize(GCM_NONCE_SIZE);

    if (RAND_bytes(
            reinterpret_cast<unsigned char*>(nonce.data()),
            nonce.size()) != 1)
    {
        nonce.clear();
        return {};
    }

    EVP_CIPHER_CTX* ctx =
        EVP_CIPHER_CTX_new();

    if (ctx == nullptr)
    {
        return {};
    }

    QByteArray ciphertext;
    ciphertext.resize(
        plaintext.size()
        + EVP_MAX_BLOCK_LENGTH
        );

    int outLength = 0;
    int finalLength = 0;

    /*
     * 初始化 AES-256-GCM
     */
    if (EVP_EncryptInit_ex(
            ctx,
            EVP_aes_256_gcm(),
            nullptr,
            nullptr,
            nullptr) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }

    /*
     * 设置 nonce
     */
    if (EVP_EncryptInit_ex(
            ctx,
            nullptr,
            nullptr,
            reinterpret_cast<const unsigned char*>(
                key.constData()),
            reinterpret_cast<const unsigned char*>(
                nonce.constData())) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }

    /*
     * 加密明文
     */
    if (EVP_EncryptUpdate(
            ctx,
            reinterpret_cast<unsigned char*>(
                ciphertext.data()),
            &outLength,
            reinterpret_cast<const unsigned char*>(
                plaintext.constData()),
            plaintext.size()) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }

    /*
     * 完成加密
     */
    if (EVP_EncryptFinal_ex(
            ctx,
            reinterpret_cast<unsigned char*>(
                ciphertext.data()) + outLength,
            &finalLength) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }

    ciphertext.resize(
        outLength + finalLength
        );

    /*
     * 获取 GCM authentication tag
     */
    tag.resize(GCM_TAG_SIZE);

    if (EVP_CIPHER_CTX_ctrl(
            ctx,
            EVP_CTRL_GCM_GET_TAG,
            GCM_TAG_SIZE,
            tag.data()) != 1)
    {
        tag.clear();
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }

    EVP_CIPHER_CTX_free(ctx);

    return ciphertext;
}

QByteArray CryptoManager::decrypt(
    const QByteArray& ciphertext,
    const QByteArray& key,
    const QByteArray& nonce,
    const QByteArray& tag)
{
    if (key.size() != AES_KEY_SIZE)
    {
        return {};
    }

    if (nonce.size() != GCM_NONCE_SIZE)
    {
        return {};
    }

    if (tag.size() != GCM_TAG_SIZE)
    {
        return {};
    }

    EVP_CIPHER_CTX* ctx =
        EVP_CIPHER_CTX_new();

    if (ctx == nullptr)
    {
        return {};
    }

    QByteArray plaintext;
    plaintext.resize(
        ciphertext.size()
        + EVP_MAX_BLOCK_LENGTH
        );

    int outLength = 0;
    int finalLength = 0;

    /*
     * 初始化 AES-256-GCM
     */
    if (EVP_DecryptInit_ex(
            ctx,
            EVP_aes_256_gcm(),
            nullptr,
            nullptr,
            nullptr) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }

    /*
     * 设置 nonce 和 key
     */
    if (EVP_DecryptInit_ex(
            ctx,
            nullptr,
            nullptr,
            reinterpret_cast<const unsigned char*>(
                key.constData()),
            reinterpret_cast<const unsigned char*>(
                nonce.constData())) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }

    /*
     * 解密 ciphertext
     */
    if (EVP_DecryptUpdate(
            ctx,
            reinterpret_cast<unsigned char*>(
                plaintext.data()),
            &outLength,
            reinterpret_cast<const unsigned char*>(
                ciphertext.constData()),
            ciphertext.size()) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }

    /*
     * 设置 authentication tag。
     *
     * GCM 最重要的一点：
     * 如果密文被篡改，最终认证会失败。
     */
    if (EVP_CIPHER_CTX_ctrl(
            ctx,
            EVP_CTRL_GCM_SET_TAG,
            GCM_TAG_SIZE,
            const_cast<char*>(tag.constData())) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }

    /*
     * 完成解密并验证 tag。
     *
     * 返回值 != 1 表示认证失败。
     */
    if (EVP_DecryptFinal_ex(
            ctx,
            reinterpret_cast<unsigned char*>(
                plaintext.data()) + outLength,
            &finalLength) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);

        // 认证失败，不返回任何明文
        return {};
    }

    plaintext.resize(
        outLength + finalLength
        );

    EVP_CIPHER_CTX_free(ctx);

    return plaintext;
}