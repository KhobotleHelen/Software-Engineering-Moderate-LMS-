#include "services/AuthService.h"

#include "common/Errors.h"
#include "common/Validation.h"

namespace lms {

AuthService::AuthService(IUserRepository& users, const IPasswordHasher& hasher)
    : users_(users), hasher_(hasher), dummy_(hasher.hash("timing-equaliser")) {}

std::unique_ptr<User> AuthService::login(const std::string& username, const std::string& password) const {
    static const char* const kMsg = "Invalid username or password.";
    if (username.empty() || username.size() > 64 || password.size() > 128) throw AuthenticationError(kMsg);
    auto user = users_.findByUsername(username);
    if (!user) {
        hasher_.verify(password, dummy_.hash, dummy_.salt);
        throw AuthenticationError(kMsg);
    }
    const bool ok = hasher_.verify(password, user->passwordHash(), user->salt());
    if (!ok || !user->isActive()) throw AuthenticationError(kMsg);
    return user;
}

void AuthService::changePassword(User& user, const std::string& oldPassword, const std::string& newPassword) {
    if (!hasher_.verify(oldPassword, user.passwordHash(), user.salt()))
        throw AuthenticationError("The current password is incorrect.");
    validation::requirePassword(newPassword);
    const HashedPassword h = hasher_.hash(newPassword);
    user.setCredentials(h.hash, h.salt);
    users_.update(user);
}

}  // namespace lms
