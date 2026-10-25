// Administrator account management (FR-03).
#pragma once
#include "data/Repositories.h"
#include "security/PasswordHasher.h"
#include "services/AccessControl.h"

namespace lms {

class UserService {
public:
    UserService(IUserRepository& users, const IPasswordHasher& hasher, const AccessControl& access)
        : users_(users), hasher_(hasher), access_(access) {}

    // First-run bootstrap: allowed only while the user table is empty.
    Id createInitialAdministrator(const std::string& username, const std::string& password);
    Id createUser(const User& actor, const std::string& username, const std::string& password, Role role);
    void setActive(const User& actor, Id userId, bool active);
    void resetPassword(const User& actor, Id userId, const std::string& newPassword);
    std::vector<std::unique_ptr<User>> listUsers(const User& actor);

private:
    IUserRepository& users_;
    const IPasswordHasher& hasher_;
    const AccessControl& access_;
};

}  // namespace lms
