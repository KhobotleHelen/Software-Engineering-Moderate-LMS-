// FR-07, FR-08, FR-10: submission, resubmission and lateness (Use Case 1 "Submit assignment").
#include "TestEnv.h"

using namespace lms;
using lms::testing::Env;

TEST(Submit, OnTimeSubmissionIsRecorded) {  // main flow
    Env e;
    e.at(2026, 11, 15);
    const auto r = e.ctx.submissions.submit(*e.s1, e.assignmentId, "essay.pdf");
    EXPECT_EQ(r.outcome, SubmitOutcome::OnTime);
    EXPECT_EQ(r.submission.status, SubmissionStatus::Submitted);
    const auto stored = e.ctx.submissionsRepo.find(e.assignmentId, e.s1->id());
    ASSERT_TRUE(stored);
    EXPECT_EQ(stored->filePath, "essay.pdf");
    EXPECT_EQ(stored->submittedAt, e.clock.now());
}

struct LateCase {
    std::int64_t secondsAfterDeadline;
    SubmitOutcome outcome;
    SubmissionStatus status;
    int days;
};
class SubmitBoundary : public ::testing::TestWithParam<LateCase> {};

TEST_P(SubmitBoundary, DeadlineAndPenaltyBoundaries) {
    Env e;
    const auto c = GetParam();
    e.clock.set(e.deadline().plusSeconds(c.secondsAfterDeadline));
    const auto r = e.ctx.submissions.submit(*e.s1, e.assignmentId, "f.pdf");
    EXPECT_EQ(r.outcome, c.outcome);
    EXPECT_EQ(r.submission.status, c.status);
    EXPECT_EQ(r.lateDays, c.days);
    EXPECT_FALSE(r.message.empty());
}

INSTANTIATE_TEST_SUITE_P(
    Boundaries, SubmitBoundary,
    ::testing::Values(
        LateCase{-1, SubmitOutcome::OnTime, SubmissionStatus::Submitted, 0},
        LateCase{0, SubmitOutcome::OnTime, SubmissionStatus::Submitted, 0},  // exactly at deadline
        LateCase{1, SubmitOutcome::Late, SubmissionStatus::Late, 1},         // 1 second late
        LateCase{86400, SubmitOutcome::Late, SubmissionStatus::Late, 1},
        LateCase{2 * 86400 + 1, SubmitOutcome::Late, SubmissionStatus::Late, 3},
        LateCase{3 * 86400, SubmitOutcome::Late, SubmissionStatus::Late, 3},  // last day with marks
        LateCase{3 * 86400 + 1, SubmitOutcome::LateOverLimit, SubmissionStatus::Late,
                 4},  // accepted, but zero
        LateCase{20 * 86400, SubmitOutcome::LateOverLimit, SubmissionStatus::Late, 20}));

TEST(Submit, ResubmissionBeforeDeadlineReplacesTheSameRecord) {  // FR-08
    Env e;
    e.at(2026, 11, 15);
    const Id first = e.ctx.submissions.submit(*e.s1, e.assignmentId, "v1.pdf").submission.id;
    e.at(2026, 11, 16);
    const auto r = e.ctx.submissions.submit(*e.s1, e.assignmentId, "v2.pdf");
    EXPECT_EQ(r.outcome, SubmitOutcome::Resubmitted);
    EXPECT_EQ(r.submission.id, first);
    EXPECT_EQ(e.ctx.submissionsRepo.listByAssignment(e.assignmentId).size(), 1u);
    EXPECT_EQ(e.ctx.submissionsRepo.findById(first)->filePath, "v2.pdf");
    EXPECT_EQ(e.ctx.submissionsRepo.findById(first)->submittedAt, e.clock.now());
}

TEST(Submit, ResubmissionAfterDeadlineIsRejectedAndOldVersionKept) {
    Env e;
    e.at(2026, 11, 15);
    e.ctx.submissions.submit(*e.s1, e.assignmentId, "v1.pdf");
    e.clock.set(e.deadline().plusSeconds(1));
    EXPECT_THROW(e.ctx.submissions.submit(*e.s1, e.assignmentId, "v2.pdf"), BusinessRuleError);
    EXPECT_EQ(e.ctx.submissionsRepo.find(e.assignmentId, e.s1->id())->filePath, "v1.pdf");
}

TEST(Submit, LateFirstSubmissionCannotBeResubmitted) {
    Env e;
    e.clock.set(e.deadline().plusSeconds(100));
    e.ctx.submissions.submit(*e.s1, e.assignmentId, "late.pdf");
    EXPECT_THROW(e.ctx.submissions.submit(*e.s1, e.assignmentId, "later.pdf"), BusinessRuleError);
}

TEST(Submit, ResubmissionAfterMarkingIsRejected) {
    Env e;
    const Id sid = e.submitAt(DateTime::parse("2026-11-15 10:00"), *e.s1);
    e.ctx.assessment.recordMark(*e.lecturer, sid, 70, "ok", "");
    e.at(2026, 11, 16);
    EXPECT_THROW(e.ctx.submissions.submit(*e.s1, e.assignmentId, "again.pdf"), BusinessRuleError);
}

TEST(Submit, ExtensionMovesTheDeadlineSoLateWorkBecomesOnTime) {  // FR-09 + FR-10
    Env e;
    e.ctx.assignments.grantExtension(*e.lecturer, e.assignmentId, e.s1->id(), e.deadline().plusDays(2),
                                     "Family emergency");
    e.clock.set(e.deadline().plusDays(1));
    EXPECT_EQ(e.ctx.submissions.submit(*e.s1, e.assignmentId, "f.pdf").outcome, SubmitOutcome::OnTime);
    // student without extension is late at the same moment
    EXPECT_EQ(e.ctx.submissions.submit(*e.s2, e.assignmentId, "g.pdf").outcome, SubmitOutcome::Late);
}

TEST(Submit, ExtensionGrantedAfterLateSubmissionAllowsResubmission) {
    Env e;
    e.clock.set(e.deadline().plusDays(1));
    e.ctx.submissions.submit(*e.s1, e.assignmentId, "late.pdf");
    e.ctx.assignments.grantExtension(*e.lecturer, e.assignmentId, e.s1->id(), e.deadline().plusDays(3),
                                     "Accepted reason");
    const auto r = e.ctx.submissions.submit(*e.s1, e.assignmentId, "better.pdf");
    EXPECT_EQ(r.outcome, SubmitOutcome::Resubmitted);
    EXPECT_EQ(r.submission.status, SubmissionStatus::Submitted);
}

TEST(Submit, ExceptionFlows) {
    Env e;
    e.at(2026, 11, 15);
    EXPECT_THROW(e.ctx.submissions.submit(*e.outsider, e.assignmentId, "f"),
                 AuthorizationError);  // not enrolled
    EXPECT_THROW(e.ctx.submissions.submit(*e.s1, 99999, "f"), NotFoundError);
    EXPECT_THROW(e.ctx.submissions.submit(*e.s1, e.assignmentId, ""), ValidationError);
    EXPECT_THROW(e.ctx.submissions.submit(*e.s1, e.assignmentId, std::string(300, 'x')), ValidationError);
    EXPECT_TRUE(e.ctx.submissionsRepo.listByAssignment(e.assignmentId).empty());  // nothing was saved
    EXPECT_THROW(e.ctx.submissions.effectiveDeadline(*e.outsider, e.assignmentId), AuthorizationError);
    EXPECT_THROW(e.ctx.submissions.effectiveDeadline(*e.s1, 99999), NotFoundError);
}
