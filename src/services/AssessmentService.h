// Marking, moderation, release and audit (FR-11, FR-12, FR-13, NFR-08).
// Sequence diagram "Moderate and release marks"; state machine "Submission lifecycle".
#pragma once
#include "common/Clock.h"
#include "data/Database.h"
#include "data/Repositories.h"
#include "services/AccessControl.h"
#include "services/DeadlineResolver.h"
#include "strategy/LatePenalty.h"

namespace lms {

struct SubmissionView {
    Submission submission;
    std::string studentUsername;
    std::string assignmentTitle;
    std::string courseName;
    double maxMark = 0.0;
    int lateDays = 0;
    std::optional<Moderation> latestModeration;
};

struct MarkResult {
    double rawMark = 0.0;
    double finalMark = 0.0;
    double penalty = 0.0;
    int lateDays = 0;
};

struct AuditEntry {
    MarkAudit audit;
    std::string changedByUsername;
};

class AssessmentService {
public:
    AssessmentService(IAssignmentRepository& assignments, ISubmissionRepository& submissions,
                      ICourseRepository& courses, IUserRepository& users, IMarkAuditRepository& audit,
                      IModerationRepository& moderation, const DeadlineResolver& deadlines,
                      const ILatePenaltyStrategy& penalty, ITransactionRunner& tx,
                      const AccessControl& access, const IClock& clock)
        : assignments_(assignments),
          submissions_(submissions),
          courses_(courses),
          users_(users),
          audit_(audit),
          moderation_(moderation),
          deadlines_(deadlines),
          penalty_(penalty),
          tx_(tx),
          access_(access),
          clock_(clock) {}

    // Lecturer. rawMark is the mark for the quality of the work; the late penalty is applied by the system.
    MarkResult recordMark(const User& lecturer, Id submissionId, double rawMark, const std::string& feedback,
                          const std::string& reason);
    std::vector<SubmissionView> submissionsForAssignment(const User& lecturer, Id assignmentId);

    // Head of Department.
    std::vector<SubmissionView> awaitingModeration(const User& hod);
    void moderate(const User& hod, Id submissionId, ModerationOutcome outcome, const std::string& comments);
    void release(const User& hod, Id submissionId);
    int releaseAllApproved(const User& hod, Id courseId);

    // Student: unreleased marks and feedback are never returned.
    std::vector<SubmissionView> resultsForStudent(const User& student);

    std::vector<AuditEntry> auditTrail(const User& actor, Id submissionId);

private:
    SubmissionView toView(const Submission& s);
    Submission loadSubmission(Id id);
    Assignment assignmentOf(const Submission& s);

    IAssignmentRepository& assignments_;
    ISubmissionRepository& submissions_;
    ICourseRepository& courses_;
    IUserRepository& users_;
    IMarkAuditRepository& audit_;
    IModerationRepository& moderation_;
    const DeadlineResolver& deadlines_;
    const ILatePenaltyStrategy& penalty_;
    ITransactionRunner& tx_;
    const AccessControl& access_;
    const IClock& clock_;
};

}  // namespace lms
