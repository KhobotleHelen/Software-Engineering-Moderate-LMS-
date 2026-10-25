#include "services/UserService.h"

#include "common/Errors.h"
#include "common/Validation.h"

namespace lms {

Id UserService::createInitialAdministrator(const std::string& username, const std::string& password) {
    if (users_.count() != 0) throw BusinessRuleError("The system has already been set up.");
    validation::requireUsername(username);
    validation::requirePassword(password);
    const HashedPassword h = hasher_.hash(password);
    return users_.add(*UserFactory::create(Role::Administrator, 0, username, h.hash, h.salt, true));
}

Id UserService::createUser(const User& actor, const std::string& username, const std::string& password,
                           Role role) {
    access_.require(actor, Permission::ManageUsers);
    validation::requireUsername(username);
    validation::requirePassword(password);
    if (users_.findByUsername(username)) throw ValidationError("That username is already taken.");
    const HashedPassword h = hasher_.hash(password);
    return users_.add(*UserFactory::create(role, 0, username, h.hash, h.salt, true));
}

void UserService::setActive(const User& actor, Id userId, bool active) {
    access_.require(actor, Permission::ManageUsers);
    if (userId == actor.id() && !active) throw BusinessRuleError("You cannot deactivate your own account.");
    auto user = users_.findById(userId);
    if (!user) throw NotFoundError("User not found.");
    user->setActive(active);
    users_.update(*user);
}

void UserService::resetPassword(const User& actor, Id userId, const std::string& newPassword) {
    access_.require(actor, Permission::ManageUsers);
    validation::requirePassword(newPassword);
    auto user = users_.findById(userId);
    if (!user) throw NotFoundError("User not found.");
    const HashedPassword h = hasher_.hash(newPassword);
    user->setCredentials(h.hash, h.salt);
    users_.update(*user);
}

std::vector<std::unique_ptr<User>> UserService::listUsers(const User& actor) {
    access_.require(actor, Permission::ManageUsers);
    return users_.listAll();
}

}  // namespace lms
