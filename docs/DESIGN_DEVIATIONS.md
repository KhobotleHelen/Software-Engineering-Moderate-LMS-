# Where the implementation differs from the Phase 3 design (and why)

Update your Phase 3 diagrams/data dictionary to match, or cite this list in the report ("design changes found during
implementation"). Markers will check consistency between artefacts.

| # | Change | Reason | Artefacts to update |
|---|---|---|---|
| 1 | `Department.head_id` (nullable FK to User) added | Phase 3 requires HoDs to be checked against "the relevant departments" but has no column linking an HoD to a department | ERD, data dictionary, SQL schema, class diagram (`Department.headId`), class->table mapping |
| 2 | Triggers `trg_submission_mark_max_*` | Phase 3 allowed "validation or trigger" for mark <= max_mark; both are implemented (defence in depth) | SQL schema |
| 3 | Seven indexes | NFR-02 (2 s with 800 students) | SQL schema |
| 4 | `UserFactory` (Factory pattern) | Needed to rebuild the right `User` subclass from a database row; reduces `switch` duplication | Class diagram + patterns section (third pattern) |
| 5 | Component services split as: `AuthService`+`AccessControl`, `UserService`+`AcademicService`, `AssignmentService`, `SubmissionService`, `AssessmentService` | Direct realisation of the Phase 3 component diagram (Authentication/access control, Course and user management, Assignment, Submission, Assessment and moderation) | Class diagram (service classes) |
| 6 | Helpers: `DeadlineResolver`, `IClock`, `IPasswordHasher`, `ITransactionRunner`, `AppContext` | SRP/DIP: one place for "effective deadline", testable time, replaceable hashing, atomic mark+audit, composition root | Class diagram / component notes |
| 7 | Late penalty is computed when the lecturer **marks** (from `submitted_at` and the effective deadline); `Submission.mark` stores the *final* mark; raw mark and penalty are written into `MarkAudit.reason` | Keeps the schema unchanged and makes every audit row self-explanatory | Sequence diagram (note), SRS rule text |
| 8 | A part-day late counts as a full day; 1 s late = 1 day; exactly at the deadline = on time; > 3 days accepted but mark = 0 | Closes the SRS open question (Conflict 3) | SRS business rule |
| 9 | Resubmission: unlimited number, only before the effective deadline, only while status is Submitted/Late; only the latest version is kept | Closes SRS open question (Conflict 4); Phase 3 schema note says version history would need a new table | SRS business rule, limitations |
| 10 | Extension must be later than the original deadline; effective deadline = later of the two; one extension per student per assignment (re-granting replaces it) | Matches `UNIQUE(assignment_id, student_id)` | SRS business rule |
| 11 | v1.0 student assignment list shows the *original* deadline; the *effective* deadline is shown when submitting | Leaves CR1 ("show extensions on the student's assignment list") as a genuine change for Phase 8 | Maintenance impact analysis |
| 12 | HoD cannot edit marks: "CorrectionRequired" returns work to the lecturer (status stays `Marked`); once `Moderated` a mark is locked | Matches the Phase 3 state machine | Limitations |
| 13 | Extra self-service/admin functions: change password, reset password, activate/deactivate | Needed to operate the system safely; NFR-04 | SRS (add as Should requirements if you wish) |

## Known limitations (state them honestly in the report/viva)
- Submissions store a **file path**, not a copy of the file; on a network the path must be to a shared location.
- No lock-out after repeated failed logins and no session time-out (console app; mitigated by Argon2id cost).
- The SQLite file is not encrypted; protect it with operating-system permissions and backups.
- Moderated/released marks cannot be re-opened (a "reopen with reason" use case would be a sensible future change).
- Windows password-hiding and console behaviour were written against the platform API but only tested on Linux.
