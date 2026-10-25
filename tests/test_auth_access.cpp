// FR-01, FR-02, NFR-04, NFR-05: authentication and role-based access control.
#include "TestEnv.h"
#include "data/Database.h"

using namespace lms;
using lms::testing::Env;

TEST(Auth, LoginSucceedsWithCorrectCredentials) {
    Env e;
    auto u = e.ctx.auth.login("lect1", Env::kPw);
    ASSERT_TRUE(u);
    EXPECT_EQ(u->role(), Role::Lecturer);
}

TEST(Auth, FailureMessageIsIdenticalForWrongPasswordAndUnknownUser) {
    Env e;
    std::string m1, m2;
    try {
        e.ctx.auth.login("lect1", "wrong-password");
    } catch (const AuthenticationError& x) {
        m1 = x.what();
    }
    try {
        e.ctx.auth.login("nobody", "whatever1");
    } catch (const AuthenticationError& x) {
        m2 = x.what();
    }
    EXPECT_FALSE(m1.empty());
    EXPECT_EQ(m1, m2);  // no user enumeration
}

TEST(Auth, DeactivatedAccountCannotLogIn) {
    Env e;
    e.ctx.userService.setActive(*e.admin, e.s1->id(), false);
    EXPECT_THROW(e.ctx.auth.login("stud1", Env::kPw), AuthenticationError);
    e.ctx.userService.setActive(*e.admin, e.s1->id(), true);
    EXPECT_NO_THROW(e.ctx.auth.login("stud1", Env::kPw));
}

TEST(Auth, MalformedLoginInputIsRejectedWithoutCrash) {
    Env e;
    EXPECT_THROW(e.ctx.auth.login("", ""), AuthenticationError);
    EXPECT_THROW(e.ctx.auth.login(std::string(1000, 'a'), "x"), AuthenticationError);
    EXPECT_THROW(e.ctx.auth.login("lect1", std::string(5000, 'p')), AuthenticationError);
    EXPECT_THROW(e.ctx.auth.login("' OR 1=1 --", "' OR 1=1 --"), AuthenticationError);
}

TEST(Auth, PasswordsAreStoredOnlyAsSaltedHashes) {  // NFR-04
    Env e;
    Statement st(e.db, "SELECT password_hash, salt FROM User");
    int rows = 0;
    while (st.step()) {
        ++rows;
        EXPECT_EQ(st.text(0).find(Env::kPw), std::string::npos);
        EXPECT_EQ(st.text(0).size(), 64u);
        EXPECT_EQ(st.text(1).size(), 32u);
    }
    EXPECT_EQ(rows, 8);
    // Same password for every account, yet every stored hash differs because every salt differs.
    Statement distinct(e.db, "SELECT COUNT(DISTINCT password_hash) FROM User");
    distinct.step();
    EXPECT_EQ(distinct.integer(0), 8);
}

TEST(Auth, ChangePasswordRequiresCurrentPasswordAndPolicy) {
    Env e;
    auto u = e.ctx.auth.login("stud1", Env::kPw);
    EXPECT_THROW(e.ctx.auth.changePassword(*u, "wrong-current", "NewPassword1"), AuthenticationError);
    EXPECT_THROW(e.ctx.auth.changePassword(*u, Env::kPw, "short"), ValidationError);
    e.ctx.auth.changePassword(*u, Env::kPw, "NewPassword1");
    EXPECT_THROW(e.ctx.auth.login("stud1", Env::kPw), AuthenticationError);
    EXPECT_NO_THROW(e.ctx.auth.login("stud1", "NewPassword1"));
}

TEST(Access, PermissionMatrixForAllFourRoles) {
    Env e;
    struct Row {
        Permission p;
        bool student, lecturer, hod, admin;
    };
    const Row rows[] = {
        {Permission::ManageUsers, false, false, false, true},
        {Permission::ManageAcademicData, false, false, false, true},
        {Permission::CreateAssignment, false, true, false, false},
        {Permission::ViewAssignments, true, true, true, true},
        {Permission::SubmitWork, true, false, false, false},
        {Permission::GrantExtension, false, true, false, false},
        {Permission::MarkWork, false, true, false, false},
        {Permission::ModerateMarks, false, false, true, false},
        {Permission::ReleaseMarks, false, false, true, false},
        {Permission::ViewOwnMarks, true, false, false, false},
        {Permission::ViewAuditLog, false, true, true, false},
    };
    for (const auto& r : rows) {
        EXPECT_EQ(e.s1->can(r.p), r.student);
        EXPECT_EQ(e.lecturer->can(r.p), r.lecturer);
        EXPECT_EQ(e.hod->can(r.p), r.hod);
        EXPECT_EQ(e.admin->can(r.p), r.admin);
    }
}

TEST(Access, RoleNamesAreReportedPolymorphically) {
    Env e;
    EXPECT_EQ(e.s1->roleName(), "Student");
    EXPECT_EQ(e.lecturer->roleName(), "Lecturer");
    EXPECT_EQ(e.hod->roleName(), "HoD");
    EXPECT_EQ(e.admin->roleName(), "Administrator");
}

TEST(Access, ForbiddenActionsThrowAuthorizationError) {
    Env e;
    EXPECT_THROW(e.ctx.userService.createUser(*e.lecturer, "newuser", "Password123", Role::Student),
                 AuthorizationError);
    EXPECT_THROW(e.ctx.academic.createDepartment(*e.s1, "Hack"), AuthorizationError);
    EXPECT_THROW(e.ctx.assignments.createAssignment(*e.s1, e.courseId, "t", "d", e.deadline(), 10),
                 AuthorizationError);
    EXPECT_THROW(e.ctx.submissions.submit(*e.lecturer, e.assignmentId, "f"), AuthorizationError);
    EXPECT_THROW(e.ctx.assessment.awaitingModeration(*e.lecturer), AuthorizationError);
    EXPECT_THROW(e.ctx.assessment.resultsForStudent(*e.lecturer), AuthorizationError);
    EXPECT_THROW(e.ctx.userService.listUsers(*e.hod), AuthorizationError);
}

TEST(Access, ScopedChecksStopCrossCourseAndCrossDepartmentAccess) {
    Env e;
    EXPECT_THROW(e.ctx.assignments.createAssignment(*e.otherLecturer, e.courseId, "t", "d", e.deadline(), 10),
                 AuthorizationError);
    EXPECT_THROW(e.ctx.submissions.submit(*e.outsider, e.assignmentId, "f"), AuthorizationError);
    EXPECT_THROW(e.ctx.assignments.listForCourse(*e.otherLecturer, e.courseId), AuthorizationError);
    EXPECT_THROW(e.ctx.assignments.listForCourse(*e.otherHod, e.courseId), AuthorizationError);
    EXPECT_NO_THROW(e.ctx.assignments.listForCourse(*e.hod, e.courseId));
    EXPECT_THROW(e.ctx.assignments.listForCourse(*e.admin, e.courseId), AuthorizationError);
}

TEST(Access, DeactivatedActorIsRefused) {
    Env e;
    e.s1->setActive(false);
    EXPECT_THROW(e.ctx.assignments.listForStudent(*e.s1), AuthorizationError);
}
