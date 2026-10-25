// Integration tests of the data-access layer against a real (in-memory) SQLite database.
#include "TestEnv.h"
#include "data/Database.h"

using namespace lms;
using lms::testing::Env;

TEST(Repositories, UserRoundTripPreservesRoleAndPolymorphism) {
    Env e;
    auto u = e.ctx.users.findByUsername("stud1");
    ASSERT_TRUE(u);
    EXPECT_EQ(u->role(), Role::Student);
    EXPECT_NE(dynamic_cast<Student*>(u.get()), nullptr);
    EXPECT_NE(dynamic_cast<HeadOfDepartment*>(e.ctx.users.findByUsername("hod1").get()), nullptr);
    EXPECT_EQ(e.ctx.users.findById(9999), nullptr);
    EXPECT_EQ(e.ctx.users.count(), 8);
}

TEST(Repositories, DuplicateUsernameRejectedByDatabaseConstraint) {
    Env e;
    const auto dup = UserFactory::create(Role::Student, 0, "stud1", "h", "s", true);
    EXPECT_THROW(e.ctx.users.add(*dup), DatabaseError);
}

TEST(Repositories, ParameterisedQueriesNeutraliseSqlInjection) {
    Env e;
    EXPECT_EQ(e.ctx.users.findByUsername("x' OR '1'='1"), nullptr);
    EXPECT_EQ(e.ctx.users.findByUsername("'; DROP TABLE User; --"), nullptr);
    EXPECT_EQ(e.ctx.users.count(), 8);  // table still intact
}

TEST(Repositories, ForeignKeysAreEnforced) {
    Env e;
    EXPECT_THROW(e.ctx.enrolments.add(99999, e.courseId, e.clock.now()), DatabaseError);
    EXPECT_THROW(e.ctx.courses.add(99999, "Orphan course"), DatabaseError);
}

TEST(Repositories, DatabaseRejectsMarkAboveAssignmentMaximum) {
    Env e;
    Submission s;
    s.assignmentId = e.assignmentId;
    s.studentId = e.s1->id();
    s.submittedAt = e.clock.now();
    s.filePath = "f";
    s.mark = 101.0;  // max is 100: trigger must fire even if the application check were bypassed
    EXPECT_THROW(e.ctx.submissionsRepo.add(s), DatabaseError);
    s.mark = 100.0;
    EXPECT_NO_THROW(e.ctx.submissionsRepo.add(s));
    auto stored = e.ctx.submissionsRepo.find(e.assignmentId, e.s1->id());
    ASSERT_TRUE(stored);
    stored->mark = 100.5;
    EXPECT_THROW(e.ctx.submissionsRepo.update(*stored), DatabaseError);
}

TEST(Repositories, OneSubmissionPerStudentPerAssignment) {
    Env e;
    Submission s;
    s.assignmentId = e.assignmentId;
    s.studentId = e.s1->id();
    s.submittedAt = e.clock.now();
    s.filePath = "f";
    e.ctx.submissionsRepo.add(s);
    EXPECT_THROW(e.ctx.submissionsRepo.add(s), DatabaseError);
}

TEST(Repositories, TransactionRollsBackOnException) {
    Env e;
    EXPECT_THROW(e.db.run([&] {
        e.ctx.departments.add("Temporary");
        throw std::runtime_error("boom");
    }),
                 std::runtime_error);
    for (const auto& d : e.ctx.departments.listAll()) EXPECT_NE(d.name, "Temporary");
}

TEST(Repositories, NestedTransactionsJoinOuterAndRollBackTogether) {
    Env e;
    EXPECT_THROW(e.db.run([&] {
        e.ctx.departments.add("Outer");
        e.db.run([&] { e.ctx.departments.add("Inner"); });
        throw std::runtime_error("fail after nested");
    }),
                 std::runtime_error);
    for (const auto& d : e.ctx.departments.listAll()) {
        EXPECT_NE(d.name, "Outer");
        EXPECT_NE(d.name, "Inner");
    }
    EXPECT_NO_THROW(e.db.run([&] { e.ctx.departments.add("After"); }));  // connection still usable
}

TEST(Repositories, ExtensionUpsertKeepsOneRowPerStudentAssignment) {
    Env e;
    Extension x;
    x.assignmentId = e.assignmentId;
    x.studentId = e.s1->id();
    x.grantedBy = e.lecturer->id();
    x.revisedDeadline = DateTime::parse("2026-11-25 12:00");
    x.reason = "ill";
    e.ctx.extensions.upsert(x);
    x.revisedDeadline = DateTime::parse("2026-11-27 12:00");
    e.ctx.extensions.upsert(x);
    const auto found = e.ctx.extensions.find(e.assignmentId, e.s1->id());
    ASSERT_TRUE(found);
    EXPECT_EQ(found->revisedDeadline.toString(), "2026-11-27 12:00:00");
    EXPECT_FALSE(e.ctx.extensions.find(e.assignmentId, e.s2->id()));
}

TEST(Repositories, ListingQueriesRespectScope) {
    Env e;
    EXPECT_EQ(e.ctx.courses.listByStudent(e.s1->id()).size(), 1u);
    EXPECT_EQ(e.ctx.courses.listByStudent(e.outsider->id()).size(), 0u);
    EXPECT_EQ(e.ctx.courses.listByLecturer(e.lecturer->id()).size(), 1u);
    EXPECT_EQ(e.ctx.assignmentsRepo.listForStudent(e.s1->id()).size(), 1u);
    EXPECT_EQ(e.ctx.assignmentsRepo.listForStudent(e.outsider->id()).size(), 0u);
    EXPECT_EQ(e.ctx.enrolments.studentIdsOfCourse(e.courseId).size(), 2u);
}

TEST(Database, SchemaInitialisationIsIdempotentAndBadPathFails) {
    Database db(":memory:");
    EXPECT_NO_THROW(db.initialiseSchema());
    EXPECT_NO_THROW(db.initialiseSchema());
    EXPECT_THROW(Database("/nonexistent-dir/xyz/lms.db"), DatabaseError);
    EXPECT_THROW(db.execute("THIS IS NOT SQL"), DatabaseError);
}
