// System/acceptance tests: the real console UI driven by scripted keystrokes against a real SQLite DB.
#include <sstream>

#include "TestEnv.h"
#include "ui/ConsoleApp.h"

using namespace lms;
using lms::testing::Env;

namespace {

std::string run(AppContext& ctx, const IClock& clock, const std::string& script) {
    std::istringstream in(script);
    std::ostringstream out;
    ConsoleApp app(ctx, in, out, clock);
    app.run();
    return out.str();
}

bool has(const std::string& hay, const std::string& needle) {
    return hay.find(needle) != std::string::npos;
}

}  // namespace

TEST(SystemTest, FirstRunSetupCreatesAdministratorThenAllowsLogin) {
    Database db(":memory:");
    db.initialiseSchema();
    SodiumPasswordHasher h(SodiumPasswordHasher::Profile::Fast);
    SystemClock clock;
    StandardLatePenalty p;
    AppContext ctx(db, h, clock, p);
    const std::string out = run(ctx, clock,
                                "root1\nPassword123\nDifferent99\n"  // mismatch -> retry
                                "root1\nPassword123\nPassword123\n"  // success
                                "root1\nPassword123\n0\n\n");        // log in, log out, exit
    EXPECT_TRUE(has(out, "do not match"));
    EXPECT_TRUE(has(out, "Administrator account created"));
    EXPECT_TRUE(has(out, "Welcome, root1 (Administrator)"));
    EXPECT_EQ(ctx.users.count(), 1);
}

TEST(SystemTest, AdministratorSetsUpTheWholeSchoolThroughTheMenus) {  // FR-03, FR-04
    Database db(":memory:");
    db.initialiseSchema();
    SodiumPasswordHasher h(SodiumPasswordHasher::Profile::Fast);
    SystemClock clock;
    StandardLatePenalty p;
    AppContext ctx(db, h, clock, p);
    const std::string out = run(ctx, clock,
                                "root\nPassword123\nPassword123\n"
                                "root\nPassword123\n"
                                "1\nlect\nLecturer\nPassword123\n"  // user 2
                                "1\nhodx\nHoD\nPassword123\n"       // user 3
                                "1\nstu\nStudent\nPassword123\n"    // user 4
                                "1\nbad\nJanitor\nPassword123\n"    // invalid role -> friendly error
                                "5\nSciences\n"                     // dept 1
                                "6\n1\n3\n"                         // head
                                "7\n1\nMaths\n"                     // course 1
                                "8\n1\n2\n"                         // lecturer
                                "9\n4\n1\n"                         // enrol
                                "9\n4\n1\n"                         // duplicate enrol -> error
                                "3\n4\nn\n"                         // deactivate student
                                "2\n10\n0\n\n");
    EXPECT_TRUE(has(out, "Account created with number 2"));
    EXPECT_TRUE(has(out, "Unknown role"));
    EXPECT_TRUE(has(out, "Head of Department set"));
    EXPECT_TRUE(has(out, "Lecturer assigned"));
    EXPECT_TRUE(has(out, "Student enrolled"));
    EXPECT_TRUE(has(out, "already enrolled"));
    EXPECT_TRUE(has(out, "DEACTIVATED"));
    EXPECT_TRUE(ctx.courses.isLecturerOf(1, 2));
    EXPECT_TRUE(ctx.enrolments.exists(4, 1));
}

TEST(SystemTest, EndToEndSubmitMarkModerateReleaseThroughTheUi) {  // UC1 + UC3 + Principal's rule
    Env e;  // ids: lect1=2, hod1=4, stud1=6; assignment 1 exists
    const std::string login = "Password123\n";
    std::string out = run(e.ctx, e.clock, "stud1\n" + login + "3\n1\nwork.pdf\ny\ny\n4\n0\n\n");
    EXPECT_TRUE(has(out, "Your deadline for this assignment: 2026-11-20 23:59:00"));
    EXPECT_TRUE(has(out, "Submission received on time"));
    EXPECT_TRUE(has(out, "not released yet"));

    out = run(e.ctx, e.clock, "lect1\n" + login + "5\n1\n6\n1\n80\nGood work\n\n0\n\n");
    EXPECT_TRUE(has(out, "final mark 80.00"));
    EXPECT_TRUE(has(out, "only after the Head of Department"));

    out = run(e.ctx, e.clock, "stud1\n" + login + "4\n0\n\n");  // marked but NOT released: still hidden
    EXPECT_TRUE(has(out, "not released yet"));
    EXPECT_FALSE(has(out, "80.00"));
    EXPECT_FALSE(has(out, "Good work"));

    out = run(e.ctx, e.clock, "hod1\n" + login + "1\n2\n1\na\nLooks fine\n3\n1\n0\n\n");
    EXPECT_TRUE(has(out, "AWAITING REVIEW"));
    EXPECT_TRUE(has(out, "Approved."));
    EXPECT_TRUE(has(out, "Mark released to the student"));

    out = run(e.ctx, e.clock, "stud1\n" + login + "4\n0\n\n");
    EXPECT_TRUE(has(out, "80.00 / 100.00"));
    EXPECT_TRUE(has(out, "Feedback: Good work"));
}

TEST(SystemTest, LateSubmissionAndPenaltyVisibleToLecturerThroughUi) {  // FR-10
    Env e;
    e.clock.set(e.deadline().plusDays(2).plusSeconds(-30));  // 2 days late
    std::string out = run(e.ctx, e.clock, "stud1\nPassword123\n3\n1\nwork.pdf\ny\ny\n0\n\n");
    EXPECT_TRUE(has(out, "2 day(s) late"));
    out = run(e.ctx, e.clock, "lect1\nPassword123\n6\n1\n90\nGood\n\n0\n\n");
    EXPECT_TRUE(has(out, "late 2 day(s), penalty 20.00, final mark 70.00"));
}

TEST(SystemTest, ExceptionFlowsNeverCrashAndShowFriendlyMessages) {  // NFR-06
    Env e;
    e.submitAt(DateTime::parse("2026-11-15 10:00"), *e.s1);
    e.at(2026, 11, 16);
    EXPECT_TRUE(
        has(run(e.ctx, e.clock, "lect1\nwrong-password\n"), "Invalid username or password."));  // then EOF
    const std::string out = run(e.ctx, e.clock,
                                "lect1\nPassword123\n"
                                "99\n"                             // invalid menu number
                                "abc\n"                            // not a number
                                "6\n1\n150\nfb\n\n"                // mark above maximum
                                "6\nxyz\n"                         // non-numeric id
                                "6\n1\n50\n\n\n"                   // missing feedback
                                "2\n1\nTitle\nText\nnot-a-date\n"  // invalid date
                                "6\n999\n50\nfb\n\n"               // unknown submission
                                "0\n\n");
    EXPECT_TRUE(has(out, "Please choose a number from the menu"));
    EXPECT_TRUE(has(out, "greater than the assignment maximum"));
    EXPECT_TRUE(has(out, "Please enter a valid number"));
    EXPECT_TRUE(has(out, "Feedback must not be empty"));
    EXPECT_TRUE(has(out, "Invalid date or time"));
    EXPECT_TRUE(has(out, "Submission not found"));
    EXPECT_TRUE(has(out, "You have been logged out"));
}

TEST(SystemTest, AccessIsLimitedByRoleThroughTheUi) {  // FR-02, NFR-05
    Env e;
    const std::string stu = run(e.ctx, e.clock, "stud1\nPassword123\n0\n\n");
    EXPECT_FALSE(has(stu, "Create an assignment"));
    EXPECT_FALSE(has(stu, "Create a user account"));
    const std::string lec = run(e.ctx, e.clock, "lect1\nPassword123\n0\n\n");
    EXPECT_TRUE(has(lec, "Lecturer menu"));
    EXPECT_FALSE(has(lec, "Release approved marks"));
    // a lecturer who tries to mark another department's work is refused
    e.ctx.assignments.createAssignment(*e.otherLecturer, e.otherCourseId, "H1", "d",
                                       DateTime::parse("2026-12-01 08:00"), 10);
    e.ctx.academic.enrolStudent(*e.admin, e.s1->id(), e.otherCourseId);
    e.ctx.submissions.submit(*e.s1, 2, "h.pdf");
    const std::string denied = run(e.ctx, e.clock, "lect1\nPassword123\n6\n1\n5\nfb\n\n6\n1\n0\n\n");
    EXPECT_FALSE(has(denied, "mark saved"));
}

TEST(SystemTest, DatabaseFailureIsReportedWithoutCrashOrInternalDetails) {  // NFR-06
    Env e;
    e.db.execute("DROP TABLE Submission;");  // simulate a corrupted database
    const std::string out = run(e.ctx, e.clock, "stud1\nPassword123\n2\n0\n\n");
    EXPECT_TRUE(has(out, "A database error occurred"));
    EXPECT_FALSE(has(out, "no such table"));  // internals are logged, not shown
    EXPECT_TRUE(has(out, "You have been logged out"));
    std::remove("lms_error.log");
}

TEST(SystemTest, PasswordChangeThroughUi) {
    Env e;
    run(e.ctx, e.clock, "stud1\nPassword123\n5\nPassword123\nNewSecret99\nNewSecret99\n0\n\n");
    EXPECT_THROW(e.ctx.auth.login("stud1", "Password123"), AuthenticationError);
    EXPECT_NO_THROW(e.ctx.auth.login("stud1", "NewSecret99"));
}

TEST(SystemTest, ExtensionFlowThroughUi) {  // FR-09 end to end
    Env e;
    std::string out =
        run(e.ctx, e.clock, "lect1\nPassword123\n4\n1\n6\n2026-11-25 12:00\nHospital visit\n0\n\n");
    EXPECT_TRUE(has(out, "Extension recorded"));
    e.clock.set(DateTime::parse("2026-11-22 10:00"));
    out = run(e.ctx, e.clock, "stud1\nPassword123\n3\n1\nwork.pdf\ny\ny\n0\n\n");
    EXPECT_TRUE(has(out, "Your deadline for this assignment: 2026-11-25 12:00:00"));
    EXPECT_TRUE(has(out, "received on time"));
}
