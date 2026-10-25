#include "services/SubmissionService.h"

#include "common/Errors.h"
#include "common/Validation.h"

namespace lms {

DateTime SubmissionService::effectiveDeadline(const User& student, Id assignmentId) {
    access_.require(student, Permission::SubmitWork);
    const auto a = assignments_.findById(assignmentId);
    if (!a) throw NotFoundError("Assignment not found.");
    access_.requireEnrolled(student, a->courseId);
    return deadlines_.effectiveDeadline(*a, student.id());
}

SubmitResult SubmissionService::submit(const User& student, Id assignmentId, const std::string& filePath) {
    access_.require(student, Permission::SubmitWork);
    const auto assignment = assignments_.findById(assignmentId);
    if (!assignment) throw NotFoundError("Assignment not found.");
    access_.requireEnrolled(student, assignment->courseId);
    validation::requireNonEmpty(filePath, "File path", 260);

    const DateTime now = clock_.now();
    const DateTime deadline = deadlines_.effectiveDeadline(*assignment, student.id());
    const int lateDays = penalty_.lateDays(deadline, now);

    SubmitResult result;
    result.lateDays = lateDays;
    tx_.run([&] {
        auto existing = submissions_.find(assignmentId, student.id());
        if (existing) {
            if (existing->status != SubmissionStatus::Submitted && existing->status != SubmissionStatus::Late)
                throw BusinessRuleError(
                    "This work has already been marked and can no longer be resubmitted.");
            if (lateDays > 0)
                throw BusinessRuleError(
                    "The deadline has passed. Work can only be resubmitted before the deadline.");
            existing->filePath = filePath;
            existing->submittedAt = now;
            existing->status = SubmissionStatus::Submitted;
            submissions_.update(*existing);
            result.submission = *existing;
            result.outcome = SubmitOutcome::Resubmitted;
            result.message = "Your earlier submission was replaced by this version.";
        } else {
            Submission s;
            s.assignmentId = assignmentId;
            s.studentId = student.id();
            s.submittedAt = now;
            s.filePath = filePath;
            s.status = lateDays > 0 ? SubmissionStatus::Late : SubmissionStatus::Submitted;
            s.id = submissions_.add(s);
            result.submission = s;
            if (lateDays == 0) {
                result.outcome = SubmitOutcome::OnTime;
                result.message = "Submission received on time.";
            } else if (penalty_.exceedsLimit(lateDays)) {
                result.outcome = SubmitOutcome::LateOverLimit;
                result.message =
                    "Submission received, but it is " + std::to_string(lateDays) +
                    " day(s) late (more than the allowed limit). It will receive a mark of zero.";
            } else {
                result.outcome = SubmitOutcome::Late;
                result.message = "Submission received " + std::to_string(lateDays) +
                                 " day(s) late. A late penalty will be applied when it is marked.";
            }
        }
    });
    return result;
}

}  // namespace lms
