#include "crypto.hpp"

#include <openssl/evp.h>
#include <openssl/rand.h>

namespace clog::utils {

std::string encrypt(const std::string &password, std::istream &file) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw std::runtime_error{"Encryption failed!"};
    }

    const EVP_CIPHER *cipher = EVP_aes_128_cfb(); // You can change the cipher if needed
    unsigned char key[EVP_MAX_KEY_LENGTH];
    unsigned char iv[EVP_MAX_IV_LENGTH];

    if (EVP_BytesToKey(cipher, EVP_md5(), NULL,
                       reinterpret_cast<const unsigned char *>(password.c_str()), password.size(),
                       1, key, iv) != 16) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error{"Encryption failed!"};
    }

    if (EVP_EncryptInit_ex(ctx, cipher, nullptr, key, iv) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error{"Encryption failed!"};
    }

    unsigned char inBuffer[1024];
    unsigned char outBuffer[1024 + EVP_MAX_BLOCK_LENGTH];
    int bytesRead = 0;
    int outLength = 0;

    std::string output;

    while (file.good()) {
        file.read(reinterpret_cast<char *>(inBuffer), sizeof(inBuffer));
        bytesRead = static_cast<int>(file.gcount());

        if (EVP_EncryptUpdate(ctx, outBuffer, &outLength, inBuffer, bytesRead) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error{"Encryption failed!"};
        }

        output += std::string{outBuffer, outBuffer + outLength};
    }

    if (EVP_EncryptFinal_ex(ctx, outBuffer, &outLength) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error{"Encryption failed!"};
    }
    output += std::string{outBuffer, outBuffer + outLength};
    EVP_CIPHER_CTX_free(ctx);

    return output;
}

std::string decrypt(const std::string &password, std::istream &file) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error{"Encryption failed!"};
    }

    const EVP_CIPHER *cipher = EVP_aes_128_cfb();
    unsigned char key[EVP_MAX_KEY_LENGTH];
    unsigned char iv[EVP_MAX_IV_LENGTH];

    if (EVP_BytesToKey(cipher, EVP_md5(), NULL,
                       reinterpret_cast<const unsigned char *>(password.c_str()), password.size(),
                       1, key, iv) != 16) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error{"Encryption failed!"};
    }

    if (EVP_DecryptInit_ex(ctx, cipher, nullptr, key, iv) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error{"Encryption failed!"};
    }

    unsigned char inBuffer[1024 + EVP_MAX_BLOCK_LENGTH];
    unsigned char outBuffer[1024];
    int bytesRead = 0;
    int outLength = 0;
    std::string output;

    while (file.good()) {
        file.read(reinterpret_cast<char *>(inBuffer), sizeof(inBuffer));
        bytesRead = static_cast<int>(file.gcount());

        if (EVP_DecryptUpdate(ctx, outBuffer, &outLength, inBuffer, bytesRead) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            throw std::runtime_error{"Encryption failed!"};
        }

        output += std::string{outBuffer, outBuffer + outLength};
    }

    if (EVP_DecryptFinal_ex(ctx, outBuffer, &outLength) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error{"Encryption failed!"};
    }

    output += std::string{outBuffer, outBuffer + outLength};

    EVP_CIPHER_CTX_free(ctx);

    return output;
}
} // namespace clog::utils
