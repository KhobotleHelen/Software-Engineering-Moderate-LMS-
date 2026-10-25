// Abstract User with four polymorphic roles (Phase 3 class diagram, LSP).
#pragma once
#include <memory>
#include <set>
#include <string>

#include "domain/Types.h"

namespace lms {

class User {
public:
    User(Id id, std::string username, std::string passwordHash, std::string salt, bool active)
        : id_(id),
          username_(std::move(username)),
          passwordHash_(std::move(passwordHash)),
          salt_(std::move(salt)),
          active_(active) {}
    virtual ~User() = default;

    virtual Role role() const = 0;
    virtual const std::set<Permission>& permissions() const = 0;
    virtual std::string roleName() const { return toString(role()); }
    bool can(Permission p) const { return permissions().count(p) > 0; }

    Id id() const { return id_; }
    const std::string& username() const { return username_; }
    const std::string& passwordHash() const { return passwordHash_; }
    const std::string& salt() const { return salt_; }
    bool isActive() const { return active_; }

    void setCredentials(std::string hash, std::string salt) {
        passwordHash_ = std::move(hash);
        salt_ = std::move(salt);
    }
    void setActive(bool a) { active_ = a; }

private:
    Id id_;
    std::string username_;
    std::string passwordHash_;
    std::string salt_;
    bool active_;
};

class Student final : public User {
public:
    using User::User;
    Role role() const override { return Role::Student; }
    const std::set<Permission>& permissions() const override;
};

class Lecturer final : public User {
public:
    using User::User;
    Role role() const override { return Role::Lecturer; }
    const std::set<Permission>& permissions() const override;
};

class HeadOfDepartment final : public User {
public:
    using User::User;
    Role role() const override { return Role::HoD; }
    const std::set<Permission>& permissions() const override;
};

class Administrator final : public User {
public:
    using User::User;
    Role role() const override { return Role::Administrator; }
    const std::set<Permission>& permissions() const override;
};

// Factory pattern: single place that turns a role + stored fields into the right subclass.
class UserFactory {
public:
    static std::unique_ptr<User> create(Role role, Id id, const std::string& username,
                                        const std::string& passwordHash, const std::string& salt,
                                        bool active);
};

}  // namespace lms
