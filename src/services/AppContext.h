// Composition root: wires concrete repositories, strategy and services together (DIP in one place).
#pragma once
#include "data/Database.h"
#include "data/SqliteRepositories.h"
#include "services/AcademicService.h"
#include "services/AccessControl.h"
#include "services/AssessmentService.h"
#include "services/AssignmentService.h"
#include "services/AuthService.h"
#include "services/DeadlineResolver.h"
#include "services/SubmissionService.h"
#include "services/UserService.h"

namespace lms {

class AppContext {
public:
    AppContext(Database& db, const IPasswordHasher& hasher, const IClock& clock,
               const ILatePenaltyStrategy& penalty)
        : users(db),
          departments(db),
          courses(db),
          enrolments(db),
          assignmentsRepo(db),
          extensions(db),
          submissionsRepo(db),
          audit(db),
          moderation(db),
          deadlines(extensions),
          access(courses, enrolments, departments),
          auth(users, hasher),
          userService(users, hasher, access),
          academic(departments, courses, enrolments, users, access, clock),
          assignments(assignmentsRepo, extensions, submissionsRepo, enrolments, users, db, access, clock),
          submissions(assignmentsRepo, submissionsRepo, deadlines, penalty, db, access, clock),
          assessment(assignmentsRepo, submissionsRepo, courses, users, audit, moderation, deadlines, penalty,
                     db, access, clock) {}

    SqliteUserRepository users;
    SqliteDepartmentRepository departments;
    SqliteCourseRepository courses;
    SqliteEnrolmentRepository enrolments;
    SqliteAssignmentRepository assignmentsRepo;
    SqliteExtensionRepository extensions;
    SqliteSubmissionRepository submissionsRepo;
    SqliteMarkAuditRepository audit;
    SqliteModerationRepository moderation;
    DeadlineResolver deadlines;
    AccessControl access;
    AuthService auth;
    UserService userService;
    AcademicService academic;
    AssignmentService assignments;
    SubmissionService submissions;
    AssessmentService assessment;
};

}  // namespace lms
