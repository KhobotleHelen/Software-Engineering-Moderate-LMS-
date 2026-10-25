// FR-05, FR-06, FR-09: assignments and individual extensions.
#include "TestEnv.h"

using namespace lms;
using lms::testing::Env;

TEST(Assignments, LecturerCreatesAssignmentWithAllDetailsSaved) {  // FR-05
    Env e;
    const Id id = e.ctx.assignments.createAssignment(*e.lecturer, e.courseId, "  Lab 2  ", "Do the lab",
                                                     DateTime::parse("2026-12-01 08:00"), 50.0);
    const auto a = e.ctx.assignmentsRepo.findById(id);
    ASSERT_TRUE(a);
    EXPECT_EQ(a->title, "Lab 2");
    EXPECT_EQ(a->description, "Do the lab");
    EXPECT_DOUBLE_EQ(a->maxMark, 50.0);
    EXPECT_EQ(a->deadline.toString(), "2026-12-01 08:00:00");
    EXPECT_EQ(a->createdBy, e.lecturer->id());
}

TEST(Assignments, InvalidInputIsRejected) {
    Env e;
    const DateTime future = DateTime::parse("2026-12-01 08:00");
    auto create = [&](const std::string& t, const std::string& d, DateTime dl, double m) {
        return e.ctx.assignments.createAssignment(*e.lecturer, e.courseId, t, d, dl, m);
    };
    EXPECT_THROW(create("", "d", future, 10), ValidationError);
    EXPECT_THROW(create("t", " ", future, 10), ValidationError);
    EXPECT_THROW(create("t", "d", future, 0), ValidationError);
    EXPECT_THROW(create("t", "d", future, -5), ValidationError);
    EXPECT_THROW(create("t", "d", e.clock.now(), 10), ValidationError);                  // boundary: now
    EXPECT_THROW(create("t", "d", e.clock.now().plusSeconds(-1), 10), ValidationError);  // in the past
    EXPECT_NO_THROW(create("t", "d", e.clock.now().plusSeconds(1), 10));  // boundary: just future
}

TEST(Assignments, StudentSeesOnlyAssignmentsOfEnrolledCoursesWithStatus) {  // FR-06
    Env e;
    e.ctx.assignments.createAssignment(*e.otherLecturer, e.otherCourseId, "History essay", "d",
                                       DateTime::parse("2026-12-01 08:00"), 10);
    auto list = e.ctx.assignments.listForStudent(*e.s1);
    ASSERT_EQ(list.size(), 1u);
    EXPECT_EQ(list[0].assignment.title, "Essay 1");
    EXPECT_FALSE(list[0].submissionStatus);
    e.ctx.submissions.submit(*e.s1, e.assignmentId, "f.pdf");
    EXPECT_EQ(*e.ctx.assignments.listForStudent(*e.s1)[0].submissionStatus, SubmissionStatus::Submitted);
    EXPECT_TRUE(e.ctx.assignments.listForStudent(*e.outsider).empty());
    EXPECT_THROW(e.ctx.assignments.listForStudent(*e.lecturer), AuthorizationError);
}

TEST(Extensions, GrantedExtensionChangesOnlyThatStudentsDeadline) {  // FR-09
    Env e;
    const DateTime ext = DateTime::parse("2026-11-25 12:00");
    e.ctx.assignments.grantExtension(*e.lecturer, e.assignmentId, e.s1->id(), ext, "Hospital visit");
    EXPECT_EQ(e.ctx.submissions.effectiveDeadline(*e.s1, e.assignmentId), ext);
    EXPECT_EQ(e.ctx.submissions.effectiveDeadline(*e.s2, e.assignmentId), e.deadline());
}

TEST(Extensions, ReGrantingReplacesPreviousExtension) {
    Env e;
    e.ctx.assignments.grantExtension(*e.lecturer, e.assignmentId, e.s1->id(),
                                     DateTime::parse("2026-11-25 12:00"), "r1");
    e.ctx.assignments.grantExtension(*e.lecturer, e.assignmentId, e.s1->id(),
                                     DateTime::parse("2026-11-28 12:00"), "r2");
    EXPECT_EQ(e.ctx.submissions.effectiveDeadline(*e.s1, e.assignmentId).toString(), "2026-11-28 12:00:00");
}

TEST(Extensions, InvalidExtensionsAreRejected) {
    Env e;
    const DateTime later = DateTime::parse("2026-11-25 12:00");
    EXPECT_THROW(e.ctx.assignments.grantExtension(*e.lecturer, e.assignmentId, e.s1->id(), e.deadline(), "r"),
                 ValidationError);  // not later
    EXPECT_THROW(e.ctx.assignments.grantExtension(*e.lecturer, e.assignmentId, e.s1->id(),
                                                  e.deadline().plusSeconds(-5), "r"),
                 ValidationError);
    EXPECT_NO_THROW(e.ctx.assignments.grantExtension(*e.lecturer, e.assignmentId, e.s1->id(),
                                                     e.deadline().plusSeconds(1), "r"));  // boundary
    EXPECT_THROW(e.ctx.assignments.grantExtension(*e.lecturer, e.assignmentId, e.s1->id(), later, "  "),
                 ValidationError);  // reason required
    EXPECT_THROW(e.ctx.assignments.grantExtension(*e.lecturer, e.assignmentId, e.outsider->id(), later, "r"),
                 ValidationError);  // not enrolled
    EXPECT_THROW(e.ctx.assignments.grantExtension(*e.lecturer, e.assignmentId, e.lecturer->id(), later, "r"),
                 ValidationError);  // not a student
    EXPECT_THROW(e.ctx.assignments.grantExtension(*e.otherLecturer, e.assignmentId, e.s1->id(), later, "r"),
                 AuthorizationError);
    EXPECT_THROW(e.ctx.assignments.grantExtension(*e.s1, e.assignmentId, e.s1->id(), later, "r"),
                 AuthorizationError);
    EXPECT_THROW(e.ctx.assignments.grantExtension(*e.lecturer, 99999, e.s1->id(), later, "r"), NotFoundError);
}
