// Authentication (FR-01, NFR-04): salted Argon2id hashes, generic failure message, no user enumeration.
#pragma once
#include <memory>

#include "data/Repositories.h"
#include "security/PasswordHasher.h"

namespace lms {

class AuthService {
public:
    AuthService(IUserRepository& users, const IPasswordHasher& hasher);
    std::unique_ptr<User> login(const std::string& username, const std::string& password) const;
    void changePassword(User& user, const std::string& oldPassword, const std::string& newPassword);

private:
    IUserRepository& users_;
    const IPasswordHasher& hasher_;
    HashedPassword dummy_;  // verified against when the user does not exist, to equalise timing
};

}  // namespace lms
