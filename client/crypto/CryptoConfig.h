#ifndef CRYPTOCONFIG_H
#define CRYPTOCONFIG_H

#include <QByteArray>

    namespace CryptoConfig
{
    /*
     * AES-256 测试密钥
     *
     * 当前项目为了简化 E2EE 实现，
     * 暂时使用客户端内置的共享密钥。
     *
     * 后续如果升级到 X25519，
     * 这里会改成动态生成的会话密钥。
     */
    inline QByteArray encryptionKey()
    {
        return QByteArray::fromHex(
            "00112233445566778899aabbccddeeff"
            "00112233445566778899aabbccddeeff"
            );
    }
}

#endif // CRYPTOCONFIG_H