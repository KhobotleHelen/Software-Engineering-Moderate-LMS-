// Administrator management of departments, courses, lecturer assignment and enrolment (FR-04).
#pragma once
#include "common/Clock.h"
#include "data/Repositories.h"
#include "services/AccessControl.h"

namespace lms {

class AcademicService {
public:
    AcademicService(IDepartmentRepository& departments, ICourseRepository& courses,
                    IEnrolmentRepository& enrolments, IUserRepository& users, const AccessControl& access,
                    const IClock& clock)
        : departments_(departments),
          courses_(courses),
          enrolments_(enrolments),
          users_(users),
          access_(access),
          clock_(clock) {}

    Id createDepartment(const User& actor, const std::string& name);
    void setDepartmentHead(const User& actor, Id departmentId, Id hodUserId);
    Id createCourse(const User& actor, Id departmentId, const std::string& name);
    void assignLecturer(const User& actor, Id courseId, Id lecturerId);
    Id enrolStudent(const User& actor, Id studentId, Id courseId);

    // Listings visible to the role: Admin sees everything; others see their own scope.
    std::vector<Department> listDepartments(const User& actor);
    std::vector<Course> listCourses(const User& actor);

private:
    IDepartmentRepository& departments_;
    ICourseRepository& courses_;
    IEnrolmentRepository& enrolments_;
    IUserRepository& users_;
    const AccessControl& access_;
    const IClock& clock_;
};

}  // namespace lms
