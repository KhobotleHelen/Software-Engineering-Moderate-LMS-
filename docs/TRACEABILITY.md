# Traceability: requirement -> use case -> design -> source -> tests (Phase 4 view)

| Req | Use case | Classes / methods | Tables | Source files | Test suites (tests/) |
|---|---|---|---|---|---|
| FR-01 login | Log in | `AuthService::login`, `IPasswordHasher` | User | services/AuthService.cpp, security/SodiumPasswordHasher.cpp | `Auth.*`, `PasswordHasher.*`, `SystemTest.*` |
| FR-02 role limits | all | `User::can`, `AccessControl::*`, 4 role classes | User | domain/User.cpp, services/AccessControl.cpp | `Access.*`, `SystemTest.AccessIsLimitedByRole...` |
| FR-03 manage accounts | Manage users | `UserService` | User | services/UserService.cpp | `Admin.CreateUser...`, `Admin.Deactivate...` |
| FR-04 depts/courses/enrol | Manage academic data | `AcademicService` | Department, Course, CourseLecturer, Enrolment | services/AcademicService.cpp | `Admin.*`, `SystemTest.AdministratorSetsUp...` |
| FR-05 create assignment | Create assignment | `AssignmentService::createAssignment` | Assignment | services/AssignmentService.cpp | `Assignments.*` |
| FR-06 view assignments | View assignments | `AssignmentService::listForStudent` | Assignment, Enrolment | same | `Assignments.StudentSees...` |
| FR-07 submit | Submit assignment | `SubmissionService::submit` | Submission | services/SubmissionService.cpp | `Submit.*`, `SubmitBoundary.*` |
| FR-08 resubmit | Resubmit | same | Submission | same | `Submit.Resubmission...` |
| FR-09 extension | Grant extension | `AssignmentService::grantExtension`, `DeadlineResolver` | Extension | services/AssignmentService.cpp, DeadlineResolver.h | `Extensions.*`, `SystemTest.ExtensionFlow...` |
| FR-10 late penalty | Submit / Mark | `ILatePenaltyStrategy`, `StandardLatePenalty` | - | strategy/LatePenalty.cpp | `LateDaysBoundary.*`, `PenaltyMarks.*`, `LatePenalty.*`, `Marking.LatePenalty...` |
| FR-11 marks + feedback | Mark submission | `AssessmentService::recordMark` | Submission | services/AssessmentService.cpp | `Marking.*` |
| FR-12 moderation/release | Review and release marks | `AssessmentService::moderate/release/resultsForStudent` | Moderation, Submission | same | `Moderation.*`, `SystemTest.EndToEnd...` |
| FR-13 / NFR-08 audit | Mark / view history | `AssessmentService::recordMark`, `auditTrail`, `IMarkAuditRepository` | MarkAudit | same + data/SqliteRepositories.cpp | `Marking.ChangingAnExistingMark...`, `Marking.MarkAndAuditAreWrittenInOneTransaction` |
| FR-14 role views | all | `listCourses`, `submissionsForAssignment`, `awaitingModeration` | - | services/* | `Admin.CourseListings...`, `Marking.OnlyTheCourseLecturer...` |
| NFR-01/02 speed | - | indexes, `Database` | all | data/Schema.h | `Performance.*` |
| NFR-03 offline | - | SQLite local file, no network code | - | whole code base | (no network API used; run tests with network disabled) |
| NFR-04 hashed passwords | - | `SodiumPasswordHasher` | User | security/ | `PasswordHasher.*`, `Auth.PasswordsAreStoredOnlyAsSaltedHashes` |
| NFR-05 no forbidden access | - | `AccessControl` | - | services/AccessControl.cpp | `Access.*`, `Moderation.HoDOfAnotherDepartment...` |
| NFR-06 no crash | - | exception hierarchy, `guarded()` in UI | - | common/Errors.h, ui/ConsoleApp.cpp | `SystemTest.ExceptionFlows...`, `SystemTest.DatabaseFailure...`, `Repositories.*` |
| NFR-07 usability | - | role-specific menus | - | ui/ConsoleApp.cpp | manual: user test with one person per role (Phase 5 acceptance) |
