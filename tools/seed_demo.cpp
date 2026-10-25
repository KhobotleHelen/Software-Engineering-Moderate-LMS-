// Creates a demo/performance database: 1 admin, 2 HoDs, 5 lecturers and N students (default 800).
// Usage: lms_seed <db-file> [students]   -- every account password is "Password123".
#include <cstdlib>
#include <iostream>

#include "common/Clock.h"
#include "security/PasswordHasher.h"
#include "services/AppContext.h"
#include "strategy/LatePenalty.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: lms_seed <db-file> [students]\n";
        return 2;
    }
    const int students = argc > 2 ? std::atoi(argv[2]) : 800;
    try {
        lms::Database db(argv[1]);
        db.initialiseSchema();
        lms::SodiumPasswordHasher hasher;
        lms::SystemClock clock;
        lms::StandardLatePenalty penalty;
        lms::AppContext ctx(db, hasher, clock, penalty);
        const std::string pw = "Password123";
        ctx.userService.createInitialAdministrator("admin", pw);
        const auto admin = ctx.users.findByUsername("admin");
        const lms::Id d1 = ctx.academic.createDepartment(*admin, "Sciences");
        const lms::Id d2 = ctx.academic.createDepartment(*admin, "Humanities");
        const lms::Id h1 = ctx.userService.createUser(*admin, "hod_sci", pw, lms::Role::HoD);
        const lms::Id h2 = ctx.userService.createUser(*admin, "hod_hum", pw, lms::Role::HoD);
        ctx.academic.setDepartmentHead(*admin, d1, h1);
        ctx.academic.setDepartmentHead(*admin, d2, h2);
        std::vector<lms::Id> courses;
        const char* names[] = {"Mathematics", "Physics", "English", "History", "Geography"};
        for (int i = 0; i < 5; ++i) {
            const lms::Id c = ctx.academic.createCourse(*admin, i < 2 ? d1 : d2, names[i]);
            courses.push_back(c);
            const lms::Id l = ctx.userService.createUser(*admin, "lecturer" + std::to_string(i + 1), pw,
                                                         lms::Role::Lecturer);
            ctx.academic.assignLecturer(*admin, c, l);
        }
        db.run([&] {
            for (int i = 1; i <= students; ++i) {
                const lms::Id s =
                    ctx.userService.createUser(*admin, "student" + std::to_string(i), pw, lms::Role::Student);
                for (int k = 0; k < 3; ++k)
                    ctx.academic.enrolStudent(*admin, s, courses[static_cast<std::size_t>((i + k) % 5)]);
            }
        });
        std::cout << "Seeded " << students
                  << " students. Log in as admin / lecturer1 / hod_sci / student1 (Password123).\n";
    } catch (const std::exception& e) {
        std::cerr << "Seeding failed: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
