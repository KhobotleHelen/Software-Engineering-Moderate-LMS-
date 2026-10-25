// Role-based access control (FR-02, NFR-05). A role grants *capabilities* (Permission); the scoped
// checks below additionally verify the user belongs to the specific course/department involved,
// as required by the Phase 3 schema notes ("the role field alone does not grant permission").
#pragma once
#include "data/Repositories.h"
#include "domain/User.h"

namespace lms {

class AccessControl {
public:
    AccessControl(ICourseRepository& courses, IEnrolmentRepository& enrolments,
                  IDepartmentRepository& departments)
        : courses_(courses), enrolments_(enrolments), departments_(departments) {}

    static void require(const User& actor, Permission p);  // AuthorizationError
    void requireTeaches(const User& lecturer, Id courseId) const;
    void requireEnrolled(const User& student, Id courseId) const;
    void requireHeadOfCourseDepartment(const User& hod, Id courseId) const;
    // Lecturer who teaches the course OR HoD of its department.
    void requireCourseStaff(const User& actor, Id courseId) const;
    std::optional<Department> departmentOfHead(const User& hod) const;

private:
    ICourseRepository& courses_;
    IEnrolmentRepository& enrolments_;
    IDepartmentRepository& departments_;
};

}  // namespace lms
