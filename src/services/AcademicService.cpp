#include "services/AcademicService.h"

#include "common/Errors.h"
#include "common/Validation.h"

namespace lms {

Id AcademicService::createDepartment(const User& actor, const std::string& name) {
    access_.require(actor, Permission::ManageAcademicData);
    validation::requireNonEmpty(name, "Department name", 100);
    return departments_.add(validation::trim(name));
}

void AcademicService::setDepartmentHead(const User& actor, Id departmentId, Id hodUserId) {
    access_.require(actor, Permission::ManageAcademicData);
    if (!departments_.findById(departmentId)) throw NotFoundError("Department not found.");
    auto hod = users_.findById(hodUserId);
    if (!hod || hod->role() != Role::HoD || !hod->isActive())
        throw ValidationError("The selected user is not an active Head of Department.");
    if (auto existing = departments_.findByHead(hodUserId); existing && existing->id != departmentId)
        throw BusinessRuleError("That Head of Department already heads another department.");
    departments_.setHead(departmentId, hodUserId);
}

Id AcademicService::createCourse(const User& actor, Id departmentId, const std::string& name) {
    access_.require(actor, Permission::ManageAcademicData);
    validation::requireNonEmpty(name, "Course name", 100);
    if (!departments_.findById(departmentId)) throw NotFoundError("Department not found.");
    return courses_.add(departmentId, validation::trim(name));
}

void AcademicService::assignLecturer(const User& actor, Id courseId, Id lecturerId) {
    access_.require(actor, Permission::ManageAcademicData);
    if (!courses_.findById(courseId)) throw NotFoundError("Course not found.");
    auto lecturer = users_.findById(lecturerId);
    if (!lecturer || lecturer->role() != Role::Lecturer || !lecturer->isActive())
        throw ValidationError("The selected user is not an active lecturer.");
    courses_.addLecturer(courseId, lecturerId);
}

Id AcademicService::enrolStudent(const User& actor, Id studentId, Id courseId) {
    access_.require(actor, Permission::ManageAcademicData);
    if (!courses_.findById(courseId)) throw NotFoundError("Course not found.");
    auto student = users_.findById(studentId);
    if (!student || student->role() != Role::Student || !student->isActive())
        throw ValidationError("The selected user is not an active student.");
    if (enrolments_.exists(studentId, courseId)) throw BusinessRuleError("The student is already enrolled.");
    return enrolments_.add(studentId, courseId, clock_.now());
}

std::vector<Department> AcademicService::listDepartments(const User& actor) {
    access_.require(actor, Permission::ManageAcademicData);
    return departments_.listAll();
}

std::vector<Course> AcademicService::listCourses(const User& actor) {
    switch (actor.role()) {
        case Role::Administrator:
            return courses_.listAll();
        case Role::Lecturer:
            return courses_.listByLecturer(actor.id());
        case Role::Student:
            return courses_.listByStudent(actor.id());
        case Role::HoD: {
            const auto dept = access_.departmentOfHead(actor);
            return dept ? courses_.listByDepartment(dept->id) : std::vector<Course>{};
        }
    }
    return {};
}

}  // namespace lms
