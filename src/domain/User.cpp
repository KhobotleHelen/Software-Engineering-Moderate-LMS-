#include "domain/User.h"

namespace lms {

const std::set<Permission>& Student::permissions() const {
    static const std::set<Permission> p = {Permission::ViewAssignments, Permission::SubmitWork,
                                           Permission::ViewOwnMarks};
    return p;
}

const std::set<Permission>& Lecturer::permissions() const {
    static const std::set<Permission> p = {Permission::CreateAssignment, Permission::ViewAssignments,
                                           Permission::GrantExtension, Permission::MarkWork,
                                           Permission::ViewAuditLog};
    return p;
}

const std::set<Permission>& HeadOfDepartment::permissions() const {
    static const std::set<Permission> p = {Permission::ViewAssignments, Permission::ModerateMarks,
                                           Permission::ReleaseMarks, Permission::ViewAuditLog};
    return p;
}

const std::set<Permission>& Administrator::permissions() const {
    static const std::set<Permission> p = {Permission::ManageUsers, Permission::ManageAcademicData,
                                           Permission::ViewAssignments};
    return p;
}

std::unique_ptr<User> UserFactory::create(Role role, Id id, const std::string& username,
                                          const std::string& passwordHash, const std::string& salt,
                                          bool active) {
    switch (role) {
        case Role::Student:
            return std::make_unique<Student>(id, username, passwordHash, salt, active);
        case Role::Lecturer:
            return std::make_unique<Lecturer>(id, username, passwordHash, salt, active);
        case Role::HoD:
            return std::make_unique<HeadOfDepartment>(id, username, passwordHash, salt, active);
        case Role::Administrator:
            return std::make_unique<Administrator>(id, username, passwordHash, salt, active);
    }
    return nullptr;
}

}  // namespace lms
