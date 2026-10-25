// Assignment creation, individual extensions and assignment listings (FR-05, FR-06, FR-09).
#pragma once
#include "common/Clock.h"
#include "data/Database.h"
#include "data/Repositories.h"
#include "services/AccessControl.h"
#include "services/DeadlineResolver.h"

namespace lms {

struct StudentAssignmentView {
    Assignment assignment;
    std::optional<SubmissionStatus> submissionStatus;  // empty = not submitted yet
};

class AssignmentService {
public:
    AssignmentService(IAssignmentRepository& assignments, IExtensionRepository& extensions,
                      ISubmissionRepository& submissions, IEnrolmentRepository& enrolments,
                      IUserRepository& users, ITransactionRunner& tx, const AccessControl& access,
                      const IClock& clock)
        : assignments_(assignments),
          extensions_(extensions),
          submissions_(submissions),
          enrolments_(enrolments),
          users_(users),
          tx_(tx),
          access_(access),
          clock_(clock) {}

    Id createAssignment(const User& lecturer, Id courseId, const std::string& title,
                        const std::string& description, const DateTime& deadline, double maxMark);
    void grantExtension(const User& lecturer, Id assignmentId, Id studentId, const DateTime& revisedDeadline,
                        const std::string& reason);
    std::vector<StudentAssignmentView> listForStudent(const User& student);
    std::vector<Assignment> listForCourse(const User& staff, Id courseId);

private:
    IAssignmentRepository& assignments_;
    IExtensionRepository& extensions_;
    ISubmissionRepository& submissions_;
    IEnrolmentRepository& enrolments_;
    IUserRepository& users_;
    ITransactionRunner& tx_;
    const AccessControl& access_;
    const IClock& clock_;
};

}  // namespace lms
