#include <vector>
#include <memory>
#include <logicalaccess/myexception.hpp>
#include <logicalaccess/logs.hpp>
#include "logicalaccess/crypto/des_cipher.hpp"
#include "logicalaccess/crypto/aes_cipher.hpp"
#include "logicalaccess/crypto/cmac.hpp"
#include "logicalaccess/crypto/symmetric_key.hpp"
#include "logicalaccess/crypto/aes_symmetric_key.hpp"
#include "logicalaccess/crypto/aes_initialization_vector.hpp"
#include "logicalaccess/crypto/des_symmetric_key.hpp"
#include "logicalaccess/crypto/des_initialization_vector.hpp"
#include "logicalaccess/bufferhelper.hpp"

namespace logicalaccess
{
namespace openssl
{
ByteVector CMACCrypto::cmac(const ByteVector &key, std::string crypto,
                            const ByteVector &data, const ByteVector &iv,
                            int padding_size)
{
    ByteVector cmac, lastiv;

    std::shared_ptr<OpenSSLSymmetricCipher> cipherMAC;
    unsigned int block_size = 0;
    if (crypto == "des" || crypto == "3des")
    {
        cipherMAC.reset(new DESCipher(OpenSSLSymmetricCipher::ENC_MODE_CBC));
        block_size = 8;
    }
    else if (crypto == "aes")
    {
        cipherMAC.reset(new AESCipher(OpenSSLSymmetricCipher::ENC_MODE_CBC));
        block_size = 16;
    }
    else
    {
        THROW_EXCEPTION_WITH_LOG(LibLogicalAccessException,
                                 "Wrong crypto mechanism: " + crypto);
    }

    if (cipherMAC)
    {
        lastiv.resize(block_size, 0x00);
        if (padding_size == 0)
        {
            padding_size = block_size;
        }
        if (iv.size() > 0)
        {
            for (unsigned char i = 0; i < iv.size(); ++i)
            {
                lastiv[i] = iv[i];
            }
        }

        cmac = CMACCrypto::cmac(key, cipherMAC, block_size, data, lastiv, padding_size);
        if (cmac.size() > block_size)
        {
            cmac = ByteVector(cmac.end() - block_size, cmac.end());
        }
    }

    return cmac;
}

ByteVector CMACCrypto::cmac(const ByteVector &key,
                            std::shared_ptr<OpenSSLSymmetricCipher> cipherMAC,
                            unsigned int block_size, const ByteVector &data,
                            ByteVector lastIV, unsigned int padding_size, bool forceK2Use)
{
    std::shared_ptr<OpenSSLSymmetricCipher> cipherK1K2;

    std::shared_ptr<SymmetricKey> symkey;
    std::shared_ptr<InitializationVector> iv;
    // 3DES
    if (std::dynamic_pointer_cast<DESCipher>(cipherMAC))
    {
        cipherK1K2.reset(new DESCipher(OpenSSLSymmetricCipher::ENC_MODE_ECB));

        symkey.reset(new DESSymmetricKey(DESSymmetricKey::createFromData(key)));
        iv.reset(new DESInitializationVector(DESInitializationVector::createNull()));
    }
    // AES
    else
    {
        cipherK1K2.reset(new AESCipher(OpenSSLSymmetricCipher::ENC_MODE_ECB));

        symkey.reset(new AESSymmetricKey(AESSymmetricKey::createFromData(key)));
        iv.reset(new AESInitializationVector(AESInitializationVector::createNull()));
    }

    unsigned char Rb;

    // TDES
    if (block_size == 8)
    {
        Rb = 0x1b;
    }
    // AES
    else
    {
        Rb = 0x87;
    }

    ByteVector blankbuf;
    blankbuf.resize(block_size, 0x00);
    ByteVector L;
    cipherK1K2->cipher(blankbuf, L, *symkey.get(), *iv.get(), false);

    ByteVector K1;
    if ((L[0] & 0x80) == 0x00)
    {
        K1 = shift_string(L);
    }
    else
    {
        K1 = shift_string(L, Rb);
    }

    ByteVector K2;
    if ((K1[0] & 0x80) == 0x00)
    {
        K2 = shift_string(K1);
    }
    else
    {
        K2 = shift_string(K1, Rb);
    }

    int pad = (padding_size - (data.size() % padding_size)) % padding_size;
    if (data.size() == 0)
        pad = padding_size;

    ByteVector padded_data = data;
    if (pad > 0)
    {
        padded_data.push_back(0x80);
        if (pad > 1)
        {
            for (int i = 0; i < (pad - 1); ++i)
            {
                padded_data.push_back(0x00);
            }
        }
    }

    // XOR with K1
    if (pad == 0 && !forceK2Use)
    {
        for (unsigned int i = 0; i < K1.size(); ++i)
        {
            padded_data[padded_data.size() - K1.size() + i] = static_cast<unsigned char>(
                padded_data[padded_data.size() - K1.size() + i] ^ K1[i]);
        }
    }
    // XOR with K2
    else
    {
        for (unsigned int i = 0; i < K2.size(); ++i)
        {
            padded_data[padded_data.size() - K2.size() + i] = static_cast<unsigned char>(
                padded_data[padded_data.size() - K2.size() + i] ^ K2[i]);
        }
    }

    ByteVector ret;
    // 3DES
    if (std::dynamic_pointer_cast<DESCipher>(cipherMAC))
    {
        iv.reset(
            new DESInitializationVector(DESInitializationVector::createFromData(lastIV)));
    }
    // AES
    else
    {
        iv.reset(
            new AESInitializationVector(AESInitializationVector::createFromData(lastIV)));
    }
    cipherMAC->cipher(padded_data, ret, *symkey.get(), *iv.get(), false);

    return ret;
}

ByteVector CMACCrypto::shift_string(const ByteVector &buf, unsigned char xorparam)
{
    ByteVector ret = buf;

    for (unsigned int i = 0; i < ret.size() - 1; ++i)
    {
        ret[i] = ret[i] << 1;
        // add the carry over bit
        ret[i] = ret[i] | ((ret[i + 1] & 0x80) ? 0x01 : 0x00);
    }

    ret[ret.size() - 1] = ret[ret.size() - 1] << 1;

    if (xorparam != 0x00)
    {
        ret[ret.size() - 1] = static_cast<unsigned char>(ret[ret.size() - 1] ^ xorparam);
    }

    return ret;
}
}
}