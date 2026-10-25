#include <sodium.h>

#include <vector>

#include "common/Errors.h"
#include "security/PasswordHasher.h"

namespace lms {
namespace {

constexpr std::size_t kHashLen = 32;

std::string toHex(const unsigned char* data, std::size_t len) {
    std::string out(len * 2 + 1, '\0');
    sodium_bin2hex(out.data(), out.size(), data, len);
    out.resize(len * 2);
    return out;
}

bool fromHex(const std::string& hex, std::vector<unsigned char>& out) {
    out.assign(hex.size() / 2 + 1, 0);
    std::size_t binLen = 0;
    if (sodium_hex2bin(out.data(), out.size(), hex.c_str(), hex.size(), nullptr, &binLen, nullptr) != 0)
        return false;
    out.resize(binLen);
    return true;
}

void initSodium() {
    static const int rc = sodium_init();  // thread-safe, idempotent
    if (rc < 0) throw LmsError("Cryptographic library failed to initialise.");
}

}  // namespace

SodiumPasswordHasher::SodiumPasswordHasher(Profile profile) : profile_(profile) {
    initSodium();
}

HashedPassword SodiumPasswordHasher::hash(const std::string& password) const {
    unsigned char salt[crypto_pwhash_SALTBYTES];
    randombytes_buf(salt, sizeof salt);
    unsigned char out[kHashLen];
    const unsigned long long ops =
        profile_ == Profile::Fast ? crypto_pwhash_OPSLIMIT_MIN : crypto_pwhash_OPSLIMIT_INTERACTIVE;
    const std::size_t mem =
        profile_ == Profile::Fast ? crypto_pwhash_MEMLIMIT_MIN : crypto_pwhash_MEMLIMIT_INTERACTIVE;
    if (crypto_pwhash(out, sizeof out, password.c_str(), password.size(), salt, ops, mem,
                      crypto_pwhash_ALG_ARGON2ID13) != 0)
        throw LmsError("Not enough memory to hash the password.");
    return {toHex(out, sizeof out), toHex(salt, sizeof salt)};
}

bool SodiumPasswordHasher::verify(const std::string& password, const std::string& hashHex,
                                  const std::string& saltHex) const {
    std::vector<unsigned char> expected, salt;
    if (!fromHex(hashHex, expected) || !fromHex(saltHex, salt) || expected.size() != kHashLen ||
        salt.size() != crypto_pwhash_SALTBYTES)
        return false;
    unsigned char out[kHashLen];
    const unsigned long long ops =
        profile_ == Profile::Fast ? crypto_pwhash_OPSLIMIT_MIN : crypto_pwhash_OPSLIMIT_INTERACTIVE;
    const std::size_t mem =
        profile_ == Profile::Fast ? crypto_pwhash_MEMLIMIT_MIN : crypto_pwhash_MEMLIMIT_INTERACTIVE;
    if (crypto_pwhash(out, sizeof out, password.c_str(), password.size(), salt.data(), ops, mem,
                      crypto_pwhash_ALG_ARGON2ID13) != 0)
        return false;
    return sodium_memcmp(out, expected.data(), kHashLen) == 0;  // constant-time comparison
}

}  // namespace lms
