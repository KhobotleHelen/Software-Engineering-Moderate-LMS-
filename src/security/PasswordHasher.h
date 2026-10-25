// Password hashing abstraction (DIP). Production implementation uses libsodium Argon2id.
#pragma once
#include <string>

namespace lms {

struct HashedPassword {
    std::string hash;  // hex
    std::string salt;  // hex
};

class IPasswordHasher {
public:
    virtual ~IPasswordHasher() = default;
    virtual HashedPassword hash(const std::string& password) const = 0;
    virtual bool verify(const std::string& password, const std::string& hashHex,
                        const std::string& saltHex) const = 0;
};

// Argon2id via libsodium (crypto_pwhash) with a random 16-byte salt per password.
// Profile::Interactive is the production default; Profile::Fast exists ONLY so unit tests run quickly.
class SodiumPasswordHasher final : public IPasswordHasher {
public:
    enum class Profile { Interactive, Fast };
    explicit SodiumPasswordHasher(Profile profile = Profile::Interactive);
    HashedPassword hash(const std::string& password) const override;
    bool verify(const std::string& password, const std::string& hashHex,
                const std::string& saltHex) const override;

private:
    Profile profile_;
};

}  // namespace lms
