#include "services/AccessControl.h"

#include "common/Errors.h"

namespace lms {

void AccessControl::require(const User& actor, Permission p) {
    if (!actor.isActive() || !actor.can(p))
        throw AuthorizationError("You are not allowed to perform this action.");
}

void AccessControl::requireTeaches(const User& lecturer, Id courseId) const {
    if (lecturer.role() != Role::Lecturer || !courses_.isLecturerOf(courseId, lecturer.id()))
        throw AuthorizationError("You are not assigned to teach this course.");
}

void AccessControl::requireEnrolled(const User& student, Id courseId) const {
    if (student.role() != Role::Student || !enrolments_.exists(student.id(), courseId))
        throw AuthorizationError("You are not enrolled in this course.");
}

std::optional<Department> AccessControl::departmentOfHead(const User& hod) const {
    return departments_.findByHead(hod.id());
}

void AccessControl::requireHeadOfCourseDepartment(const User& hod, Id courseId) const {
    const auto course = courses_.findById(courseId);
    const auto dept = departmentOfHead(hod);
    if (hod.role() != Role::HoD || !course || !dept || dept->id != course->departmentId)
        throw AuthorizationError("This course is not in your department.");
}

void AccessControl::requireCourseStaff(const User& actor, Id courseId) const {
    if (actor.role() == Role::Lecturer) return requireTeaches(actor, courseId);
    if (actor.role() == Role::HoD) return requireHeadOfCourseDepartment(actor, courseId);
    throw AuthorizationError("You are not allowed to perform this action.");
}

}  // namespace lms
