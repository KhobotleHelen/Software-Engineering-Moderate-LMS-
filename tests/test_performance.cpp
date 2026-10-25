// NFR-01 / NFR-02: response time with 800 students stored. Measured on the build machine; the same
// test (ctest -R Performance) is re-run on the school's computer during acceptance testing.
#include <chrono>

#include "TestEnv.h"

using namespace lms;

namespace {
double seconds(const std::chrono::steady_clock::time_point& t0) {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
}
}  // namespace

TEST(Performance, LoginAndCommonQueriesWithin2SecondsFor800Students) {
    Database db(":memory:");
    db.initialiseSchema();
    SodiumPasswordHasher production;  // real (Interactive) Argon2id cost
    SystemClock clock;
    StandardLatePenalty penalty;
    AppContext ctx(db, production, clock, penalty);
    ctx.userService.createInitialAdministrator("admin", "Password123");
    const auto admin = ctx.users.findByUsername("admin");
    const Id dept = ctx.academic.createDepartment(*admin, "Sciences");
    const Id hod = ctx.userService.createUser(*admin, "hod", "Password123", Role::HoD);
    ctx.academic.setDepartmentHead(*admin, dept, hod);
    const Id lect = ctx.userService.createUser(*admin, "lect", "Password123", Role::Lecturer);
    std::vector<Id> courses;
    for (int i = 0; i < 5; ++i) {
        courses.push_back(ctx.academic.createCourse(*admin, dept, "Course" + std::to_string(i)));
        ctx.academic.assignLecturer(*admin, courses.back(), lect);
    }
    const auto lecturer = ctx.users.findByUsername("lect");
    const HashedPassword hp = production.hash("Password123");  // one hash reused only to speed up seeding
    const DateTime deadline = clock.now().plusDays(30);
    std::vector<Id> assignments;
    for (Id c : courses)
        assignments.push_back(ctx.assignments.createAssignment(*lecturer, c, "A", "d", deadline, 100));

    db.run([&] {
        for (int i = 0; i < 800; ++i) {
            const Id s = ctx.users.add(
                *UserFactory::create(Role::Student, 0, "s" + std::to_string(i), hp.hash, hp.salt, true));
            for (int k = 0; k < 3; ++k) {
                const std::size_t c = static_cast<std::size_t>((i + k) % 5);
                ctx.enrolments.add(s, courses[c], clock.now());
                Submission sub;
                sub.assignmentId = assignments[c];
                sub.studentId = s;
                sub.submittedAt = clock.now();
                sub.filePath = "f.pdf";
                sub.status = SubmissionStatus::Marked;
                sub.mark = 60.0;
                sub.feedback = "ok";
                ctx.submissionsRepo.add(sub);
            }
        }
    });
    ASSERT_EQ(ctx.users.count(), 803);

    auto t0 = std::chrono::steady_clock::now();
    const auto user = ctx.auth.login("s799", "Password123");
    EXPECT_LE(seconds(t0), 2.0) << "NFR-01 login";
    ASSERT_TRUE(user);

    t0 = std::chrono::steady_clock::now();
    EXPECT_EQ(ctx.assignments.listForStudent(*user).size(), 3u);
    EXPECT_LE(seconds(t0), 2.0) << "NFR-02 assignment list";

    t0 = std::chrono::steady_clock::now();
    EXPECT_EQ(ctx.assessment.resultsForStudent(*user).size(), 3u);
    EXPECT_LE(seconds(t0), 2.0) << "NFR-02 results";

    const auto hodUser = ctx.users.findById(hod);
    t0 = std::chrono::steady_clock::now();
    EXPECT_EQ(ctx.assessment.awaitingModeration(*hodUser).size(),
              2400u);  // HoD sees every mark in the department
    EXPECT_LE(seconds(t0), 2.0) << "NFR-02 HoD review list (2400 marks)";

    t0 = std::chrono::steady_clock::now();
    EXPECT_EQ(ctx.assessment.submissionsForAssignment(*lecturer, assignments[0]).size(), 480u);
    EXPECT_LE(seconds(t0), 2.0) << "NFR-02 lecturer submissions list";
}
