// SQL schema = Phase 3 schema, plus the three documented implementation changes:
//  (1) Department.head_id  - the Phase 3 design had no way to say which HoD heads which department,
//      yet requires HoDs to be checked against "the relevant departments".
//  (2) Triggers enforcing mark <= Assignment.max_mark (Phase 3 allowed trigger OR application check;
//      we do both: defence in depth).
//  (3) Indexes for NFR-02 (2 s with 800 students).
#pragma once

namespace lms {

inline constexpr const char* kSchemaSql = R"SQL(
CREATE TABLE IF NOT EXISTS User (
    user_id INTEGER PRIMARY KEY,
    username TEXT NOT NULL UNIQUE,
    password_hash TEXT NOT NULL,
    salt TEXT NOT NULL,
    role TEXT NOT NULL CHECK (role IN ('Student', 'Lecturer', 'HoD', 'Administrator')),
    active INTEGER NOT NULL DEFAULT 1 CHECK (active IN (0, 1))
);

CREATE TABLE IF NOT EXISTS Department (
    department_id INTEGER PRIMARY KEY,
    name TEXT NOT NULL UNIQUE,
    head_id INTEGER,
    FOREIGN KEY (head_id) REFERENCES User(user_id)
);

CREATE TABLE IF NOT EXISTS Course (
    course_id INTEGER PRIMARY KEY,
    department_id INTEGER NOT NULL,
    course_name TEXT NOT NULL,
    FOREIGN KEY (department_id) REFERENCES Department(department_id)
);

CREATE TABLE IF NOT EXISTS CourseLecturer (
    course_id INTEGER NOT NULL,
    lecturer_id INTEGER NOT NULL,
    PRIMARY KEY (course_id, lecturer_id),
    FOREIGN KEY (course_id) REFERENCES Course(course_id),
    FOREIGN KEY (lecturer_id) REFERENCES User(user_id)
);

CREATE TABLE IF NOT EXISTS Enrolment (
    enrolment_id INTEGER PRIMARY KEY,
    student_id INTEGER NOT NULL,
    course_id INTEGER NOT NULL,
    enrolled_at TEXT NOT NULL,
    UNIQUE (student_id, course_id),
    FOREIGN KEY (student_id) REFERENCES User(user_id),
    FOREIGN KEY (course_id) REFERENCES Course(course_id)
);

CREATE TABLE IF NOT EXISTS Assignment (
    assignment_id INTEGER PRIMARY KEY,
    course_id INTEGER NOT NULL,
    created_by INTEGER NOT NULL,
    title TEXT NOT NULL,
    description TEXT NOT NULL,
    deadline TEXT NOT NULL,
    max_mark REAL NOT NULL CHECK (max_mark > 0),
    FOREIGN KEY (course_id) REFERENCES Course(course_id),
    FOREIGN KEY (created_by) REFERENCES User(user_id)
);

CREATE TABLE IF NOT EXISTS Extension (
    extension_id INTEGER PRIMARY KEY,
    assignment_id INTEGER NOT NULL,
    student_id INTEGER NOT NULL,
    granted_by INTEGER NOT NULL,
    revised_deadline TEXT NOT NULL,
    reason TEXT NOT NULL,
    UNIQUE (assignment_id, student_id),
    FOREIGN KEY (assignment_id) REFERENCES Assignment(assignment_id),
    FOREIGN KEY (student_id) REFERENCES User(user_id),
    FOREIGN KEY (granted_by) REFERENCES User(user_id)
);

CREATE TABLE IF NOT EXISTS Submission (
    submission_id INTEGER PRIMARY KEY,
    assignment_id INTEGER NOT NULL,
    student_id INTEGER NOT NULL,
    submitted_at TEXT NOT NULL,
    file_path TEXT NOT NULL,
    status TEXT NOT NULL CHECK (status IN ('Submitted', 'Late', 'Marked', 'Moderated', 'Released')),
    mark REAL,
    feedback TEXT,
    UNIQUE (assignment_id, student_id),
    CHECK (mark IS NULL OR mark >= 0),
    FOREIGN KEY (assignment_id) REFERENCES Assignment(assignment_id),
    FOREIGN KEY (student_id) REFERENCES User(user_id)
);

CREATE TABLE IF NOT EXISTS MarkAudit (
    audit_id INTEGER PRIMARY KEY,
    submission_id INTEGER NOT NULL,
    changed_by INTEGER NOT NULL,
    old_mark REAL,
    new_mark REAL NOT NULL CHECK (new_mark >= 0),
    reason TEXT NOT NULL,
    changed_at TEXT NOT NULL,
    FOREIGN KEY (submission_id) REFERENCES Submission(submission_id),
    FOREIGN KEY (changed_by) REFERENCES User(user_id)
);

CREATE TABLE IF NOT EXISTS Moderation (
    moderation_id INTEGER PRIMARY KEY,
    submission_id INTEGER NOT NULL,
    moderator_id INTEGER NOT NULL,
    outcome TEXT NOT NULL CHECK (outcome IN ('Approved', 'CorrectionRequired')),
    comments TEXT,
    moderated_at TEXT NOT NULL,
    FOREIGN KEY (submission_id) REFERENCES Submission(submission_id),
    FOREIGN KEY (moderator_id) REFERENCES User(user_id)
);

CREATE TRIGGER IF NOT EXISTS trg_submission_mark_max_insert
BEFORE INSERT ON Submission
WHEN NEW.mark IS NOT NULL AND NEW.mark >
     (SELECT max_mark FROM Assignment WHERE assignment_id = NEW.assignment_id)
BEGIN
    SELECT RAISE(ABORT, 'mark exceeds the assignment maximum');
END;

CREATE TRIGGER IF NOT EXISTS trg_submission_mark_max_update
BEFORE UPDATE OF mark ON Submission
WHEN NEW.mark IS NOT NULL AND NEW.mark >
     (SELECT max_mark FROM Assignment WHERE assignment_id = NEW.assignment_id)
BEGIN
    SELECT RAISE(ABORT, 'mark exceeds the assignment maximum');
END;

CREATE INDEX IF NOT EXISTS idx_course_department ON Course(department_id);
CREATE INDEX IF NOT EXISTS idx_enrolment_course ON Enrolment(course_id);
CREATE INDEX IF NOT EXISTS idx_assignment_course ON Assignment(course_id);
CREATE INDEX IF NOT EXISTS idx_submission_assignment ON Submission(assignment_id);
CREATE INDEX IF NOT EXISTS idx_submission_student ON Submission(student_id);
CREATE INDEX IF NOT EXISTS idx_audit_submission ON MarkAudit(submission_id);
CREATE INDEX IF NOT EXISTS idx_moderation_submission ON Moderation(submission_id);
)SQL";

}  // namespace lms
