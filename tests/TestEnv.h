// Shared fixture: in-memory SQLite + fast hasher + controllable clock + a small school.
#pragma once
#include <gtest/gtest.h>

#include "services/AppContext.h"

namespace lms::testing {

struct Env {
    Database db{":memory:"};
    SodiumPasswordHasher hasher{SodiumPasswordHasher::Profile::Fast};
    FixedClock clock{DateTime::fromParts(2026, 11, 10, 9, 0)};
    StandardLatePenalty penalty;
    AppContext ctx;

    std::unique_ptr<User> admin, lecturer, otherLecturer, hod, otherHod, s1, s2, outsider;
    Id deptId = 0, otherDeptId = 0, courseId = 0, otherCourseId = 0, assignmentId = 0;
    static constexpr const char* kPw = "Password123";

    Env() : ctx(db, hasher, clock, penalty) {
        db.initialiseSchema();
        ctx.userService.createInitialAdministrator("admin", kPw);
        admin = ctx.users.findByUsername("admin");
        auto mk = [&](const char* name, Role r) {
            ctx.userService.createUser(*admin, name, kPw, r);
            return ctx.users.findByUsername(name);
        };
        lecturer = mk("lect1", Role::Lecturer);
        otherLecturer = mk("lect2", Role::Lecturer);
        hod = mk("hod1", Role::HoD);
        otherHod = mk("hod2", Role::HoD);
        s1 = mk("stud1", Role::Student);
        s2 = mk("stud2", Role::Student);
        outsider = mk("outsider", Role::Student);
        deptId = ctx.academic.createDepartment(*admin, "Sciences");
        otherDeptId = ctx.academic.createDepartment(*admin, "Humanities");
        ctx.academic.setDepartmentHead(*admin, deptId, hod->id());
        ctx.academic.setDepartmentHead(*admin, otherDeptId, otherHod->id());
        courseId = ctx.academic.createCourse(*admin, deptId, "Mathematics");
        otherCourseId = ctx.academic.createCourse(*admin, otherDeptId, "History");
        ctx.academic.assignLecturer(*admin, courseId, lecturer->id());
        ctx.academic.assignLecturer(*admin, otherCourseId, otherLecturer->id());
        ctx.academic.enrolStudent(*admin, s1->id(), courseId);
        ctx.academic.enrolStudent(*admin, s2->id(), courseId);
        assignmentId = ctx.assignments.createAssignment(*lecturer, courseId, "Essay 1", "Write an essay",
                                                        DateTime::fromParts(2026, 11, 20, 23, 59), 100.0);
    }

    void at(int y, int mo, int d, int h = 12, int mi = 0, int s = 0) {
        clock.set(DateTime::fromParts(y, mo, d, h, mi, s));
    }
    DateTime deadline() const { return DateTime::fromParts(2026, 11, 20, 23, 59); }
    Id submitAt(const DateTime& when, const User& student, const char* path = "work.pdf") {
        clock.set(when);
        return ctx.submissions.submit(student, assignmentId, path).submission.id;
    }
};

}  // namespace lms::testing
