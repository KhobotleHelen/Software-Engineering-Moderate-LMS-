// Submission and resubmission workflow (FR-07, FR-08, FR-10). Sequence diagram "Submit assignment".
#pragma once
#include "common/Clock.h"
#include "data/Database.h"
#include "data/Repositories.h"
#include "services/AccessControl.h"
#include "services/DeadlineResolver.h"
#include "strategy/LatePenalty.h"

namespace lms {

enum class SubmitOutcome { OnTime, Late, LateOverLimit, Resubmitted };

struct SubmitResult {
    Submission submission;
    SubmitOutcome outcome = SubmitOutcome::OnTime;
    int lateDays = 0;
    std::string message;
};

class SubmissionService {
public:
    SubmissionService(IAssignmentRepository& assignments, ISubmissionRepository& submissions,
                      const DeadlineResolver& deadlines, const ILatePenaltyStrategy& penalty,
                      ITransactionRunner& tx, const AccessControl& access, const IClock& clock)
        : assignments_(assignments),
          submissions_(submissions),
          deadlines_(deadlines),
          penalty_(penalty),
          tx_(tx),
          access_(access),
          clock_(clock) {}

    SubmitResult submit(const User& student, Id assignmentId, const std::string& filePath);
    // The deadline that applies to this student (used by the UI before submitting).
    DateTime effectiveDeadline(const User& student, Id assignmentId);

private:
    IAssignmentRepository& assignments_;
    ISubmissionRepository& submissions_;
    const DeadlineResolver& deadlines_;
    const ILatePenaltyStrategy& penalty_;
    ITransactionRunner& tx_;
    const AccessControl& access_;
    const IClock& clock_;
};

}  // namespace lms
