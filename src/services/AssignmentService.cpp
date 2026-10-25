#include "services/AssignmentService.h"

#include "common/Errors.h"
#include "common/Validation.h"

namespace lms {

Id AssignmentService::createAssignment(const User& lecturer, Id courseId, const std::string& title,
                                       const std::string& description, const DateTime& deadline,
                                       double maxMark) {
    access_.require(lecturer, Permission::CreateAssignment);
    access_.requireTeaches(lecturer, courseId);
    validation::requireNonEmpty(title, "Title", 150);
    validation::requireNonEmpty(description, "Instructions", 4000);
    validation::requirePositiveMaxMark(maxMark);
    if (deadline <= clock_.now()) throw ValidationError("The deadline must be in the future.");
    Assignment a;
    a.courseId = courseId;
    a.createdBy = lecturer.id();
    a.title = validation::trim(title);
    a.description = description;
    a.deadline = deadline;
    a.maxMark = maxMark;
    return assignments_.add(a);
}

void AssignmentService::grantExtension(const User& lecturer, Id assignmentId, Id studentId,
                                       const DateTime& revisedDeadline, const std::string& reason) {
    access_.require(lecturer, Permission::GrantExtension);
    const auto assignment = assignments_.findById(assignmentId);
    if (!assignment) throw NotFoundError("Assignment not found.");
    access_.requireTeaches(lecturer, assignment->courseId);
    const auto student = users_.findById(studentId);
    if (!student || student->role() != Role::Student || !enrolments_.exists(studentId, assignment->courseId))
        throw ValidationError("That student is not enrolled in this course.");
    validation::requireNonEmpty(reason, "Reason for the extension", 500);
    if (revisedDeadline <= assignment->deadline)
        throw ValidationError("The extended deadline must be later than the original deadline (" +
                              assignment->deadline.toString() + ").");
    Extension e;
    e.assignmentId = assignmentId;
    e.studentId = studentId;
    e.grantedBy = lecturer.id();
    e.revisedDeadline = revisedDeadline;
    e.reason = validation::trim(reason);
    tx_.run([&] { extensions_.upsert(e); });
}

std::vector<StudentAssignmentView> AssignmentService::listForStudent(const User& student) {
    access_.require(student, Permission::ViewAssignments);
    if (student.role() != Role::Student) throw AuthorizationError("Only students have an assignment list.");
    std::vector<StudentAssignmentView> out;
    for (const auto& a : assignments_.listForStudent(student.id())) {
        StudentAssignmentView v;
        v.assignment = a;
        if (const auto s = submissions_.find(a.id, student.id())) v.submissionStatus = s->status;
        out.push_back(std::move(v));
    }
    return out;
}

std::vector<Assignment> AssignmentService::listForCourse(const User& staff, Id courseId) {
    access_.require(staff, Permission::ViewAssignments);
    access_.requireCourseStaff(staff, courseId);
    return assignments_.listByCourse(courseId);
}

}  // namespace lms
