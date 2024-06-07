#include "crypto.hpp"

#include <array>
#include <openssl/evp.h>
#include <openssl/rand.h>

namespace caps_log::utils {

namespace {

const int kBufferSize = 1024;

class EvpCtx {
  public:
    EvpCtx() : m_ctx{EVP_CIPHER_CTX_new()} {
        if (m_ctx == nullptr) {
            throw std::runtime_error{"Crypto failed: failed to create context!"};
        }
    }

    EvpCtx(const EvpCtx &) = delete;
    EvpCtx(EvpCtx &&) = delete;
    EvpCtx &operator=(const EvpCtx &) = delete;
    EvpCtx &operator=(EvpCtx &&) = delete;
    ~EvpCtx() { EVP_CIPHER_CTX_free(m_ctx); }

    EVP_CIPHER_CTX *get() { return m_ctx; }

  private:
    EVP_CIPHER_CTX *m_ctx;
};

auto getKeyAndIv(const std::string &password) {
    static const int kKeyLength = 16;
    std::array<unsigned char, EVP_MAX_KEY_LENGTH> key{};
    std::array<unsigned char, EVP_MAX_IV_LENGTH> iv{}; // NOLINT(readability-identifier-length)

    if (EVP_BytesToKey(EVP_aes_128_cfb(), EVP_md5(), NULL,
                       // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                       reinterpret_cast<const unsigned char *>(password.c_str()),
                       static_cast<int>(password.size()), 1, key.data(), iv.data()) != kKeyLength) {
        throw std::runtime_error{"Crypto failed: failed to generate key and iv!"};
    }

    return std::make_pair(key, iv);
}

} // namespace

std::string encrypt(const std::string &password, std::istream &file) {
    EvpCtx ctx;

    const EVP_CIPHER *cipher = EVP_aes_128_cfb();
    const auto [key, iv] = getKeyAndIv(password);

    if (EVP_EncryptInit_ex(ctx.get(), cipher, nullptr, key.data(), iv.data()) != 1) {
        throw std::runtime_error{"Encryption failed!"};
    }

    std::array<unsigned char, kBufferSize> inBuffer{};
    std::array<unsigned char, kBufferSize + EVP_MAX_BLOCK_LENGTH> outBuffer{};
    int bytesRead = 0;
    int outLength = 0;

    std::string output;

    while (file.good()) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        file.read(reinterpret_cast<char *>(inBuffer.data()), sizeof(inBuffer));
        bytesRead = static_cast<int>(file.gcount());

        if (EVP_EncryptUpdate(ctx.get(), outBuffer.data(), &outLength, inBuffer.data(),
                              bytesRead) != 1) {
            throw std::runtime_error{"Encryption failed: failed to update context!"};
        }

        output += std::string{outBuffer.begin(), outBuffer.begin() + outLength};
    }

    if (EVP_EncryptFinal_ex(ctx.get(), outBuffer.data(), &outLength) != 1) {
        throw std::runtime_error{"Encryption failed: failed to finaleze encryption!"};
    }
    output += std::string{outBuffer.begin(), outBuffer.begin() + outLength};

    return output;
}

std::string decrypt(const std::string &password, std::istream &file) {
    EvpCtx ctx;

    const EVP_CIPHER *cipher = EVP_aes_128_cfb();
    auto [key, iv] = getKeyAndIv(password);

    if (EVP_DecryptInit_ex(ctx.get(), cipher, nullptr, key.data(), iv.data()) != 1) {
        throw std::runtime_error{"Decryption failed: failed to initialze decryption!"};
    }

    std::array<unsigned char, kBufferSize + EVP_MAX_BLOCK_LENGTH> inBuffer{};
    std::array<unsigned char, kBufferSize + EVP_MAX_BLOCK_LENGTH> outBuffer{};
    int bytesRead = 0;
    int outLength = 0;
    std::string output;

    while (file.good()) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        file.read(reinterpret_cast<char *>(inBuffer.data()), inBuffer.size());
        bytesRead = static_cast<int>(file.gcount());

        if (EVP_DecryptUpdate(ctx.get(), outBuffer.data(), &outLength, inBuffer.data(),
                              bytesRead) != 1) {
            throw std::runtime_error{"Decryption failed: failed to decrypt update!"};
        }

        output += std::string{outBuffer.begin(), outBuffer.begin() + outLength};
    }

    if (EVP_DecryptFinal_ex(ctx.get(), outBuffer.data(), &outLength) != 1) {
        throw std::runtime_error{"Decryption failed: failed to decrypt final block!"};
    }

    output += std::string{outBuffer.begin(), outBuffer.begin() + outLength};

    return output;
}
// NOLINTEND
} // namespace caps_log::utils
