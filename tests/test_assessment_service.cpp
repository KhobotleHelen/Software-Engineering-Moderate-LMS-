// FR-10..FR-13, NFR-08: marking, penalties, moderation, release and the audit log.
#include "TestEnv.h"

using namespace lms;
using lms::testing::Env;

namespace {
Id onTimeSubmission(Env& e, const User& s) {
    return e.submitAt(DateTime::parse("2026-11-15 10:00"), s);
}
}  // namespace

TEST(Marking, MarkAndFeedbackAreStoredAndFirstMarkIsAudited) {  // FR-11, FR-13
    Env e;
    const Id sid = onTimeSubmission(e, *e.s1);
    e.at(2026, 11, 22);
    const MarkResult r = e.ctx.assessment.recordMark(*e.lecturer, sid, 85.5, "Clear argument", "");
    EXPECT_DOUBLE_EQ(r.finalMark, 85.5);
    const auto s = e.ctx.submissionsRepo.findById(sid);
    EXPECT_EQ(s->status, SubmissionStatus::Marked);
    EXPECT_DOUBLE_EQ(*s->mark, 85.5);
    EXPECT_EQ(s->feedback, "Clear argument");
    const auto trail = e.ctx.assessment.auditTrail(*e.lecturer, sid);
    ASSERT_EQ(trail.size(), 1u);
    EXPECT_FALSE(trail[0].audit.oldMark);
    EXPECT_DOUBLE_EQ(trail[0].audit.newMark, 85.5);
    EXPECT_EQ(trail[0].audit.changedBy, e.lecturer->id());
    EXPECT_EQ(trail[0].changedByUsername, "lect1");
    EXPECT_EQ(trail[0].audit.changedAt, e.clock.now());
    EXPECT_NE(trail[0].audit.reason.find("Initial mark"), std::string::npos);
}

TEST(Marking, LatePenaltyIsAppliedAutomaticallyAndExplainedInTheAudit) {  // FR-10
    Env e;
    const Id sid = e.submitAt(e.deadline().plusDays(1).plusSeconds(-60), *e.s1);  // 1 day late
    const MarkResult r = e.ctx.assessment.recordMark(*e.lecturer, sid, 80, "Good", "");
    EXPECT_EQ(r.lateDays, 1);
    EXPECT_DOUBLE_EQ(r.penalty, 10.0);
    EXPECT_DOUBLE_EQ(r.finalMark, 70.0);
    const auto trail = e.ctx.assessment.auditTrail(*e.lecturer, sid);
    EXPECT_DOUBLE_EQ(trail[0].audit.newMark, 70.0);
    EXPECT_NE(trail[0].audit.reason.find("1 day(s) late"), std::string::npos);
    EXPECT_NE(trail[0].audit.reason.find("raw mark 80.00"), std::string::npos);
}

TEST(Marking, WorkMoreThanThreeDaysLateReceivesZero) {
    Env e;
    const Id sid = e.submitAt(e.deadline().plusDays(4), *e.s1);
    const MarkResult r = e.ctx.assessment.recordMark(*e.lecturer, sid, 95, "Excellent but late", "");
    EXPECT_DOUBLE_EQ(r.finalMark, 0.0);
    EXPECT_EQ(e.ctx.submissionsRepo.findById(sid)->status, SubmissionStatus::Marked);
}

TEST(Marking, ExtensionRemovesThePenalty) {
    Env e;
    e.ctx.assignments.grantExtension(*e.lecturer, e.assignmentId, e.s1->id(), e.deadline().plusDays(5),
                                     "Illness");
    const Id sid = e.submitAt(e.deadline().plusDays(2), *e.s1);
    EXPECT_DOUBLE_EQ(e.ctx.assessment.recordMark(*e.lecturer, sid, 90, "fine", "").finalMark, 90.0);
}

TEST(Marking, InvalidMarksAndFeedbackAreRejected) {
    Env e;
    const Id sid = onTimeSubmission(e, *e.s1);
    auto mark = [&](double m, const char* fb) {
        return e.ctx.assessment.recordMark(*e.lecturer, sid, m, fb, "");
    };
    EXPECT_THROW(mark(100.01, "fb"), ValidationError);
    EXPECT_THROW(mark(-1, "fb"), ValidationError);
    EXPECT_THROW(mark(50, ""), ValidationError);  // written feedback is required
    EXPECT_THROW(mark(50, "   "), ValidationError);
    EXPECT_FALSE(e.ctx.submissionsRepo.findById(sid)->mark);  // nothing saved
    EXPECT_TRUE(e.ctx.assessment.auditTrail(*e.lecturer, sid).empty());
    EXPECT_NO_THROW(mark(100, "full marks"));        // boundary: exactly max
    EXPECT_THROW(mark(0, "zero"), ValidationError);  // changing an existing mark needs a reason
    EXPECT_NO_THROW(
        e.ctx.assessment.recordMark(*e.lecturer, sid, 0, "zero", "Plagiarism found"));  // boundary: zero
}

TEST(Marking, ChangingAnExistingMarkNeedsAReasonAndIsAudited) {  // NFR-08
    Env e;
    const Id sid = onTimeSubmission(e, *e.s1);
    e.ctx.assessment.recordMark(*e.lecturer, sid, 60, "First pass", "");
    EXPECT_THROW(e.ctx.assessment.recordMark(*e.lecturer, sid, 65, "Second pass", ""), ValidationError);
    e.at(2026, 11, 23, 15, 30);
    e.ctx.assessment.recordMark(*e.lecturer, sid, 65, "Second pass", "Missed a section on first read");
    const auto trail = e.ctx.assessment.auditTrail(*e.lecturer, sid);
    ASSERT_EQ(trail.size(), 2u);
    EXPECT_DOUBLE_EQ(*trail[1].audit.oldMark, 60.0);
    EXPECT_DOUBLE_EQ(trail[1].audit.newMark, 65.0);
    EXPECT_NE(trail[1].audit.reason.find("Missed a section"), std::string::npos);
    EXPECT_EQ(trail[1].audit.changedAt.toString(), "2026-11-23 15:30:00");
}

TEST(Marking, UnchangedMarkWithNewFeedbackDoesNotCreateAuditNoise) {
    Env e;
    const Id sid = onTimeSubmission(e, *e.s1);
    e.ctx.assessment.recordMark(*e.lecturer, sid, 60, "First", "");
    e.ctx.assessment.recordMark(*e.lecturer, sid, 60, "Better wording", "");
    EXPECT_EQ(e.ctx.assessment.auditTrail(*e.lecturer, sid).size(), 1u);
    EXPECT_EQ(e.ctx.submissionsRepo.findById(sid)->feedback, "Better wording");
}

TEST(Marking, OnlyTheCourseLecturerMayMarkAndViewSubmissions) {
    Env e;
    const Id sid = onTimeSubmission(e, *e.s1);
    EXPECT_THROW(e.ctx.assessment.recordMark(*e.otherLecturer, sid, 50, "x", ""), AuthorizationError);
    EXPECT_THROW(e.ctx.assessment.recordMark(*e.hod, sid, 50, "x", ""), AuthorizationError);
    EXPECT_THROW(e.ctx.assessment.recordMark(*e.s1, sid, 100, "x", ""), AuthorizationError);
    EXPECT_THROW(e.ctx.assessment.recordMark(*e.lecturer, 99999, 50, "x", ""), NotFoundError);
    EXPECT_THROW(e.ctx.assessment.submissionsForAssignment(*e.otherLecturer, e.assignmentId),
                 AuthorizationError);
    const auto list = e.ctx.assessment.submissionsForAssignment(*e.lecturer, e.assignmentId);
    ASSERT_EQ(list.size(), 1u);
    EXPECT_EQ(list[0].studentUsername, "stud1");
    EXPECT_EQ(list[0].lateDays, 0);
}

TEST(Marking, MarkAndAuditAreWrittenInOneTransaction) {  // Phase 3 schema note
    Env e;
    const Id sid = onTimeSubmission(e, *e.s1);
    struct FailingAudit : IMarkAuditRepository {
        void add(const MarkAudit&) override { throw DatabaseError("simulated disk failure"); }
        std::vector<MarkAudit> listBySubmission(Id) override { return {}; }
    } failing;
    AssessmentService svc(e.ctx.assignmentsRepo, e.ctx.submissionsRepo, e.ctx.courses, e.ctx.users, failing,
                          e.ctx.moderation, e.ctx.deadlines, e.penalty, e.db, e.ctx.access, e.clock);
    EXPECT_THROW(svc.recordMark(*e.lecturer, sid, 77, "fb", ""), DatabaseError);
    const auto s = e.ctx.submissionsRepo.findById(sid);
    EXPECT_FALSE(s->mark);  // update was rolled back
    EXPECT_EQ(s->status, SubmissionStatus::Submitted);
    EXPECT_NO_THROW(
        e.ctx.assessment.recordMark(*e.lecturer, sid, 77, "fb", ""));  // connection healthy afterwards
}

TEST(Moderation, FullLifecycleSubmittedToReleased) {  // state machine + Use Case 3
    Env e;
    const Id sid = onTimeSubmission(e, *e.s1);
    e.ctx.assessment.recordMark(*e.lecturer, sid, 72, "Solid", "");
    auto waiting = e.ctx.assessment.awaitingModeration(*e.hod);
    ASSERT_EQ(waiting.size(), 1u);
    EXPECT_EQ(waiting[0].submission.id, sid);
    e.ctx.assessment.moderate(*e.hod, sid, ModerationOutcome::Approved, "Fine");
    EXPECT_EQ(e.ctx.submissionsRepo.findById(sid)->status, SubmissionStatus::Moderated);
    e.ctx.assessment.release(*e.hod, sid);
    EXPECT_EQ(e.ctx.submissionsRepo.findById(sid)->status, SubmissionStatus::Released);
    EXPECT_TRUE(e.ctx.assessment.awaitingModeration(*e.hod).empty());
}

TEST(Moderation, StudentNeverSeesMarkOrFeedbackBeforeRelease) {  // Principal's rule, FR-12
    Env e;
    const Id sid = onTimeSubmission(e, *e.s1);
    auto check = [&](bool visible) {
        const auto r = e.ctx.assessment.resultsForStudent(*e.s1);
        ASSERT_EQ(r.size(), 1u);
        EXPECT_EQ(r[0].submission.mark.has_value(), visible);
        EXPECT_EQ(!r[0].submission.feedback.empty(), visible);
        EXPECT_FALSE(r[0].latestModeration);  // internal notes never exposed
    };
    check(false);  // Submitted
    e.ctx.assessment.recordMark(*e.lecturer, sid, 72, "Solid", "");
    check(false);  // Marked
    e.ctx.assessment.moderate(*e.hod, sid, ModerationOutcome::Approved, "");
    check(false);  // Moderated (approved, not yet released)
    e.ctx.assessment.release(*e.hod, sid);
    check(true);  // Released
    EXPECT_DOUBLE_EQ(*e.ctx.assessment.resultsForStudent(*e.s1)[0].submission.mark, 72.0);
    EXPECT_TRUE(e.ctx.assessment.resultsForStudent(*e.s2).empty());  // other students unaffected
}

TEST(Moderation, CorrectionRequiredReturnsWorkToLecturerAndNeedsComments) {
    Env e;
    const Id sid = onTimeSubmission(e, *e.s1);
    e.ctx.assessment.recordMark(*e.lecturer, sid, 40, "Weak", "");
    EXPECT_THROW(e.ctx.assessment.moderate(*e.hod, sid, ModerationOutcome::CorrectionRequired, ""),
                 ValidationError);
    e.ctx.assessment.moderate(*e.hod, sid, ModerationOutcome::CorrectionRequired,
                              "Marking scheme not applied to Q3");
    EXPECT_EQ(e.ctx.submissionsRepo.findById(sid)->status, SubmissionStatus::Marked);
    EXPECT_THROW(e.ctx.assessment.release(*e.hod, sid), BusinessRuleError);
    const auto view = e.ctx.assessment.submissionsForAssignment(*e.lecturer, e.assignmentId)[0];
    ASSERT_TRUE(view.latestModeration);
    EXPECT_EQ(view.latestModeration->outcome, ModerationOutcome::CorrectionRequired);
    e.ctx.assessment.recordMark(*e.lecturer, sid, 55, "Re-marked", "HoD correction: Q3 scheme applied");
    e.ctx.assessment.moderate(*e.hod, sid, ModerationOutcome::Approved, "");
    e.ctx.assessment.release(*e.hod, sid);
    EXPECT_EQ(e.ctx.assessment.auditTrail(*e.hod, sid).size(), 2u);
}

TEST(Moderation, ReleaseRequiresApprovalAndWorkflowOrderIsEnforced) {
    Env e;
    const Id sid = onTimeSubmission(e, *e.s1);
    EXPECT_THROW(e.ctx.assessment.release(*e.hod, sid), BusinessRuleError);  // not even marked
    EXPECT_THROW(e.ctx.assessment.moderate(*e.hod, sid, ModerationOutcome::Approved, ""), BusinessRuleError);
    e.ctx.assessment.recordMark(*e.lecturer, sid, 60, "ok", "");
    EXPECT_THROW(e.ctx.assessment.release(*e.hod, sid), BusinessRuleError);  // marked but not approved
    e.ctx.assessment.moderate(*e.hod, sid, ModerationOutcome::Approved, "");
    EXPECT_THROW(e.ctx.assessment.moderate(*e.hod, sid, ModerationOutcome::Approved, ""),
                 BusinessRuleError);  // twice
    e.ctx.assessment.release(*e.hod, sid);
    EXPECT_THROW(e.ctx.assessment.release(*e.hod, sid), BusinessRuleError);  // twice
}

TEST(Moderation, ApprovedOrReleasedMarksCannotBeChangedByLecturer) {
    Env e;
    const Id sid = onTimeSubmission(e, *e.s1);
    e.ctx.assessment.recordMark(*e.lecturer, sid, 60, "ok", "");
    e.ctx.assessment.moderate(*e.hod, sid, ModerationOutcome::Approved, "");
    EXPECT_THROW(e.ctx.assessment.recordMark(*e.lecturer, sid, 99, "ok", "greed"), BusinessRuleError);
    e.ctx.assessment.release(*e.hod, sid);
    EXPECT_THROW(e.ctx.assessment.recordMark(*e.lecturer, sid, 99, "ok", "greed"), BusinessRuleError);
    EXPECT_DOUBLE_EQ(*e.ctx.submissionsRepo.findById(sid)->mark, 60.0);
}

TEST(Moderation, HoDOfAnotherDepartmentAndOtherRolesAreRefused) {
    Env e;
    const Id sid = onTimeSubmission(e, *e.s1);
    e.ctx.assessment.recordMark(*e.lecturer, sid, 60, "ok", "");
    EXPECT_THROW(e.ctx.assessment.moderate(*e.otherHod, sid, ModerationOutcome::Approved, ""),
                 AuthorizationError);
    EXPECT_THROW(e.ctx.assessment.moderate(*e.lecturer, sid, ModerationOutcome::Approved, ""),
                 AuthorizationError);
    EXPECT_THROW(e.ctx.assessment.release(*e.s1, sid), AuthorizationError);
    EXPECT_TRUE(e.ctx.assessment.awaitingModeration(*e.otherHod).empty());  // cannot even see it
    EXPECT_THROW(e.ctx.assessment.auditTrail(*e.otherHod, sid), AuthorizationError);
    EXPECT_THROW(e.ctx.assessment.auditTrail(*e.s1, sid), AuthorizationError);
    EXPECT_THROW(e.ctx.assessment.auditTrail(*e.otherLecturer, sid), AuthorizationError);
}

TEST(Moderation, HoDWithoutADepartmentGetsAClearError) {
    Env e;
    e.ctx.userService.createUser(*e.admin, "hod3", "Password123", Role::HoD);
    const auto h3 = e.ctx.users.findByUsername("hod3");
    EXPECT_THROW(e.ctx.assessment.awaitingModeration(*h3), BusinessRuleError);
    EXPECT_TRUE(e.ctx.academic.listCourses(*h3).empty());
}

TEST(Moderation, ReleaseAllApprovedReleasesOnlyApprovedMarksOfThatCourse) {
    Env e;
    const Id a = onTimeSubmission(e, *e.s1);
    const Id b = onTimeSubmission(e, *e.s2);
    e.ctx.assessment.recordMark(*e.lecturer, a, 60, "ok", "");
    e.ctx.assessment.recordMark(*e.lecturer, b, 70, "ok", "");
    e.ctx.assessment.moderate(*e.hod, a, ModerationOutcome::Approved, "");  // b stays only Marked
    EXPECT_EQ(e.ctx.assessment.releaseAllApproved(*e.hod, e.courseId), 1);
    EXPECT_EQ(e.ctx.submissionsRepo.findById(a)->status, SubmissionStatus::Released);
    EXPECT_EQ(e.ctx.submissionsRepo.findById(b)->status, SubmissionStatus::Marked);
    EXPECT_EQ(e.ctx.assessment.releaseAllApproved(*e.hod, e.courseId), 0);
    EXPECT_THROW(e.ctx.assessment.releaseAllApproved(*e.otherHod, e.courseId), AuthorizationError);
}
