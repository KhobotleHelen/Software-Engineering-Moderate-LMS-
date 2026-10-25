// FR-03, FR-04: administrator operations.
#include "TestEnv.h"

using namespace lms;
using lms::testing::Env;

TEST(Admin, CreateUserValidatesAndRejectsDuplicates) {
    Env e;
    EXPECT_NO_THROW(e.ctx.userService.createUser(*e.admin, "newstudent", "Password123", Role::Student));
    EXPECT_THROW(e.ctx.userService.createUser(*e.admin, "newstudent", "Password123", Role::Student),
                 ValidationError);
    EXPECT_THROW(e.ctx.userService.createUser(*e.admin, "ab", "Password123", Role::Student), ValidationError);
    EXPECT_THROW(e.ctx.userService.createUser(*e.admin, "validname", "short", Role::Student),
                 ValidationError);
}

TEST(Admin, InitialAdministratorOnlyWhileSystemIsEmpty) {
    Database db(":memory:");
    db.initialiseSchema();
    SodiumPasswordHasher h(SodiumPasswordHasher::Profile::Fast);
    SystemClock c;
    StandardLatePenalty p;
    AppContext ctx(db, h, c, p);
    EXPECT_NO_THROW(ctx.userService.createInitialAdministrator("root1", "Password123"));
    EXPECT_THROW(ctx.userService.createInitialAdministrator("root2", "Password123"), BusinessRuleError);
}

TEST(Admin, DeactivateResetPasswordAndSelfProtection) {
    Env e;
    EXPECT_THROW(e.ctx.userService.setActive(*e.admin, e.admin->id(), false), BusinessRuleError);
    EXPECT_THROW(e.ctx.userService.setActive(*e.admin, 99999, false), NotFoundError);
    e.ctx.userService.resetPassword(*e.admin, e.s1->id(), "BrandNew123");
    EXPECT_NO_THROW(e.ctx.auth.login("stud1", "BrandNew123"));
    EXPECT_THROW(e.ctx.userService.resetPassword(*e.admin, e.s1->id(), "short"), ValidationError);
    EXPECT_THROW(e.ctx.userService.resetPassword(*e.admin, 99999, "BrandNew123"), NotFoundError);
    EXPECT_EQ(e.ctx.userService.listUsers(*e.admin).size(), 8u);
}

TEST(Admin, DepartmentCourseAndHeadRules) {
    Env e;
    EXPECT_THROW(e.ctx.academic.createDepartment(*e.admin, "Sciences"), DatabaseError);  // UNIQUE name
    EXPECT_THROW(e.ctx.academic.createDepartment(*e.admin, "   "), ValidationError);
    EXPECT_THROW(e.ctx.academic.createCourse(*e.admin, 99999, "X"), NotFoundError);
    EXPECT_THROW(e.ctx.academic.setDepartmentHead(*e.admin, e.deptId, e.s1->id()),
                 ValidationError);  // not a HoD
    EXPECT_THROW(e.ctx.academic.setDepartmentHead(*e.admin, e.deptId, e.otherHod->id()),
                 BusinessRuleError);  // heads another dept
    EXPECT_THROW(e.ctx.academic.setDepartmentHead(*e.admin, 99999, e.hod->id()), NotFoundError);
    EXPECT_EQ(e.ctx.academic.listDepartments(*e.admin).size(), 2u);
}

TEST(Admin, LecturerAssignmentAndEnrolmentRules) {
    Env e;
    EXPECT_THROW(e.ctx.academic.assignLecturer(*e.admin, e.courseId, e.s1->id()), ValidationError);
    EXPECT_THROW(e.ctx.academic.assignLecturer(*e.admin, 99999, e.lecturer->id()), NotFoundError);
    EXPECT_THROW(e.ctx.academic.enrolStudent(*e.admin, e.lecturer->id(), e.courseId), ValidationError);
    EXPECT_THROW(e.ctx.academic.enrolStudent(*e.admin, e.s1->id(), e.courseId),
                 BusinessRuleError);  // duplicate
    EXPECT_THROW(e.ctx.academic.enrolStudent(*e.admin, e.s1->id(), 99999), NotFoundError);
    EXPECT_NO_THROW(e.ctx.academic.enrolStudent(*e.admin, e.outsider->id(), e.courseId));
}

TEST(Admin, CourseListingsDependOnRole) {
    Env e;
    EXPECT_EQ(e.ctx.academic.listCourses(*e.admin).size(), 2u);
    EXPECT_EQ(e.ctx.academic.listCourses(*e.lecturer).size(), 1u);
    EXPECT_EQ(e.ctx.academic.listCourses(*e.s1).size(), 1u);
    EXPECT_EQ(e.ctx.academic.listCourses(*e.hod).size(), 1u);
    EXPECT_EQ(e.ctx.academic.listCourses(*e.outsider).size(), 0u);
}
