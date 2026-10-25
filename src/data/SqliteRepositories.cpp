#include "data/SqliteRepositories.h"

namespace lms {
namespace {

std::unique_ptr<User> userFromRow(const Statement& s) {
    // columns: user_id, username, password_hash, salt, role, active
    return UserFactory::create(roleFromString(s.text(4)), s.integer(0), s.text(1), s.text(2), s.text(3),
                               s.integer(5) != 0);
}
constexpr const char* kUserCols = "user_id, username, password_hash, salt, role, active";

Department deptFromRow(const Statement& s) {
    Department d;
    d.id = s.integer(0);
    d.name = s.text(1);
    d.headId = s.optInteger(2);
    return d;
}

Course courseFromRow(const Statement& s) {
    Course c;
    c.id = s.integer(0);
    c.departmentId = s.integer(1);
    c.name = s.text(2);
    return c;
}
constexpr const char* kCourseCols = "c.course_id, c.department_id, c.course_name";

Assignment assignmentFromRow(const Statement& s) {
    Assignment a;
    a.id = s.integer(0);
    a.courseId = s.integer(1);
    a.createdBy = s.integer(2);
    a.title = s.text(3);
    a.description = s.text(4);
    a.deadline = DateTime::parse(s.text(5));
    a.maxMark = s.real(6);
    return a;
}
constexpr const char* kAssignmentCols =
    "a.assignment_id, a.course_id, a.created_by, a.title, a.description, a.deadline, a.max_mark";

Submission submissionFromRow(const Statement& s) {
    Submission x;
    x.id = s.integer(0);
    x.assignmentId = s.integer(1);
    x.studentId = s.integer(2);
    x.submittedAt = DateTime::parse(s.text(3));
    x.filePath = s.text(4);
    x.status = statusFromString(s.text(5));
    x.mark = s.optReal(6);
    x.feedback = s.text(7);
    return x;
}
constexpr const char* kSubmissionCols =
    "s.submission_id, s.assignment_id, s.student_id, s.submitted_at, s.file_path, s.status, s.mark, "
    "s.feedback";

}  // namespace

// ---- Users ---------------------------------------------------------------------------------
Id SqliteUserRepository::add(const User& user) {
    Statement st(db_,
                 "INSERT INTO User (username, password_hash, salt, role, active) VALUES (?1,?2,?3,?4,?5)");
    st.bind(1, user.username())
        .bind(2, user.passwordHash())
        .bind(3, user.salt())
        .bind(4, toString(user.role()))
        .bind(5, static_cast<std::int64_t>(user.isActive() ? 1 : 0));
    st.exec();
    return sqlite3_last_insert_rowid(db_.handle());
}

void SqliteUserRepository::update(const User& user) {
    Statement st(db_, "UPDATE User SET password_hash=?1, salt=?2, active=?3 WHERE user_id=?4");
    st.bind(1, user.passwordHash())
        .bind(2, user.salt())
        .bind(3, static_cast<std::int64_t>(user.isActive() ? 1 : 0))
        .bind(4, static_cast<std::int64_t>(user.id()));
    st.exec();
}

std::unique_ptr<User> SqliteUserRepository::findById(Id id) {
    Statement st(db_, std::string("SELECT ") + kUserCols + " FROM User WHERE user_id=?1");
    st.bind(1, static_cast<std::int64_t>(id));
    return st.step() ? userFromRow(st) : nullptr;
}

std::unique_ptr<User> SqliteUserRepository::findByUsername(const std::string& username) {
    Statement st(db_, std::string("SELECT ") + kUserCols + " FROM User WHERE username=?1");
    st.bind(1, username);
    return st.step() ? userFromRow(st) : nullptr;
}

std::vector<std::unique_ptr<User>> SqliteUserRepository::listAll() {
    Statement st(db_, std::string("SELECT ") + kUserCols + " FROM User ORDER BY role, username");
    std::vector<std::unique_ptr<User>> out;
    while (st.step()) out.push_back(userFromRow(st));
    return out;
}

std::int64_t SqliteUserRepository::count() {
    Statement st(db_, "SELECT COUNT(*) FROM User");
    st.step();
    return st.integer(0);
}

// ---- Departments ---------------------------------------------------------------------------
Id SqliteDepartmentRepository::add(const std::string& name) {
    Statement st(db_, "INSERT INTO Department (name) VALUES (?1)");
    st.bind(1, name);
    st.exec();
    return sqlite3_last_insert_rowid(db_.handle());
}

std::optional<Department> SqliteDepartmentRepository::findById(Id id) {
    Statement st(db_, "SELECT department_id, name, head_id FROM Department WHERE department_id=?1");
    st.bind(1, static_cast<std::int64_t>(id));
    if (st.step()) return deptFromRow(st);
    return std::nullopt;
}

std::vector<Department> SqliteDepartmentRepository::listAll() {
    Statement st(db_, "SELECT department_id, name, head_id FROM Department ORDER BY name");
    std::vector<Department> out;
    while (st.step()) out.push_back(deptFromRow(st));
    return out;
}

std::optional<Department> SqliteDepartmentRepository::findByHead(Id hodId) {
    Statement st(db_, "SELECT department_id, name, head_id FROM Department WHERE head_id=?1");
    st.bind(1, static_cast<std::int64_t>(hodId));
    if (st.step()) return deptFromRow(st);
    return std::nullopt;
}

void SqliteDepartmentRepository::setHead(Id departmentId, Id hodId) {
    Statement st(db_, "UPDATE Department SET head_id=?1 WHERE department_id=?2");
    st.bind(1, static_cast<std::int64_t>(hodId)).bind(2, static_cast<std::int64_t>(departmentId));
    st.exec();
}

// ---- Courses -------------------------------------------------------------------------------
Id SqliteCourseRepository::add(Id departmentId, const std::string& name) {
    Statement st(db_, "INSERT INTO Course (department_id, course_name) VALUES (?1,?2)");
    st.bind(1, static_cast<std::int64_t>(departmentId)).bind(2, name);
    st.exec();
    return sqlite3_last_insert_rowid(db_.handle());
}

std::optional<Course> SqliteCourseRepository::findById(Id id) {
    Statement st(db_, std::string("SELECT ") + kCourseCols + " FROM Course c WHERE c.course_id=?1");
    st.bind(1, static_cast<std::int64_t>(id));
    if (st.step()) return courseFromRow(st);
    return std::nullopt;
}

std::vector<Course> SqliteCourseRepository::listAll() {
    Statement st(db_, std::string("SELECT ") + kCourseCols + " FROM Course c ORDER BY c.course_name");
    std::vector<Course> out;
    while (st.step()) out.push_back(courseFromRow(st));
    return out;
}

std::vector<Course> SqliteCourseRepository::listByDepartment(Id departmentId) {
    Statement st(db_, std::string("SELECT ") + kCourseCols +
                          " FROM Course c WHERE c.department_id=?1 ORDER BY c.course_name");
    st.bind(1, static_cast<std::int64_t>(departmentId));
    std::vector<Course> out;
    while (st.step()) out.push_back(courseFromRow(st));
    return out;
}

std::vector<Course> SqliteCourseRepository::listByLecturer(Id lecturerId) {
    Statement st(db_, std::string("SELECT ") + kCourseCols +
                          " FROM Course c JOIN CourseLecturer cl ON cl.course_id=c.course_id"
                          " WHERE cl.lecturer_id=?1 ORDER BY c.course_name");
    st.bind(1, static_cast<std::int64_t>(lecturerId));
    std::vector<Course> out;
    while (st.step()) out.push_back(courseFromRow(st));
    return out;
}

std::vector<Course> SqliteCourseRepository::listByStudent(Id studentId) {
    Statement st(db_, std::string("SELECT ") + kCourseCols +
                          " FROM Course c JOIN Enrolment e ON e.course_id=c.course_id"
                          " WHERE e.student_id=?1 ORDER BY c.course_name");
    st.bind(1, static_cast<std::int64_t>(studentId));
    std::vector<Course> out;
    while (st.step()) out.push_back(courseFromRow(st));
    return out;
}

void SqliteCourseRepository::addLecturer(Id courseId, Id lecturerId) {
    Statement st(db_, "INSERT OR IGNORE INTO CourseLecturer (course_id, lecturer_id) VALUES (?1,?2)");
    st.bind(1, static_cast<std::int64_t>(courseId)).bind(2, static_cast<std::int64_t>(lecturerId));
    st.exec();
}

bool SqliteCourseRepository::isLecturerOf(Id courseId, Id lecturerId) {
    Statement st(db_, "SELECT 1 FROM CourseLecturer WHERE course_id=?1 AND lecturer_id=?2");
    st.bind(1, static_cast<std::int64_t>(courseId)).bind(2, static_cast<std::int64_t>(lecturerId));
    return st.step();
}

// ---- Enrolments ----------------------------------------------------------------------------
Id SqliteEnrolmentRepository::add(Id studentId, Id courseId, const DateTime& at) {
    Statement st(db_, "INSERT INTO Enrolment (student_id, course_id, enrolled_at) VALUES (?1,?2,?3)");
    st.bind(1, static_cast<std::int64_t>(studentId))
        .bind(2, static_cast<std::int64_t>(courseId))
        .bind(3, at.toString());
    st.exec();
    return sqlite3_last_insert_rowid(db_.handle());
}

bool SqliteEnrolmentRepository::exists(Id studentId, Id courseId) {
    Statement st(db_, "SELECT 1 FROM Enrolment WHERE student_id=?1 AND course_id=?2");
    st.bind(1, static_cast<std::int64_t>(studentId)).bind(2, static_cast<std::int64_t>(courseId));
    return st.step();
}

std::vector<Id> SqliteEnrolmentRepository::studentIdsOfCourse(Id courseId) {
    Statement st(db_, "SELECT student_id FROM Enrolment WHERE course_id=?1 ORDER BY student_id");
    st.bind(1, static_cast<std::int64_t>(courseId));
    std::vector<Id> out;
    while (st.step()) out.push_back(st.integer(0));
    return out;
}

// ---- Assignments ---------------------------------------------------------------------------
Id SqliteAssignmentRepository::add(const Assignment& a) {
    Statement st(db_,
                 "INSERT INTO Assignment (course_id, created_by, title, description, deadline, max_mark)"
                 " VALUES (?1,?2,?3,?4,?5,?6)");
    st.bind(1, static_cast<std::int64_t>(a.courseId))
        .bind(2, static_cast<std::int64_t>(a.createdBy))
        .bind(3, a.title)
        .bind(4, a.description)
        .bind(5, a.deadline.toString())
        .bind(6, a.maxMark);
    st.exec();
    return sqlite3_last_insert_rowid(db_.handle());
}

std::optional<Assignment> SqliteAssignmentRepository::findById(Id id) {
    Statement st(db_,
                 std::string("SELECT ") + kAssignmentCols + " FROM Assignment a WHERE a.assignment_id=?1");
    st.bind(1, static_cast<std::int64_t>(id));
    if (st.step()) return assignmentFromRow(st);
    return std::nullopt;
}

std::vector<Assignment> SqliteAssignmentRepository::listByCourse(Id courseId) {
    Statement st(db_, std::string("SELECT ") + kAssignmentCols +
                          " FROM Assignment a WHERE a.course_id=?1 ORDER BY a.deadline, a.assignment_id");
    st.bind(1, static_cast<std::int64_t>(courseId));
    std::vector<Assignment> out;
    while (st.step()) out.push_back(assignmentFromRow(st));
    return out;
}

std::vector<Assignment> SqliteAssignmentRepository::listForStudent(Id studentId) {
    Statement st(db_, std::string("SELECT ") + kAssignmentCols +
                          " FROM Assignment a JOIN Enrolment e ON e.course_id=a.course_id"
                          " WHERE e.student_id=?1 ORDER BY a.deadline, a.assignment_id");
    st.bind(1, static_cast<std::int64_t>(studentId));
    std::vector<Assignment> out;
    while (st.step()) out.push_back(assignmentFromRow(st));
    return out;
}

// ---- Extensions ----------------------------------------------------------------------------
void SqliteExtensionRepository::upsert(const Extension& e) {
    Statement st(db_,
                 "INSERT INTO Extension (assignment_id, student_id, granted_by, revised_deadline, reason)"
                 " VALUES (?1,?2,?3,?4,?5)"
                 " ON CONFLICT(assignment_id, student_id) DO UPDATE SET"
                 " granted_by=excluded.granted_by, revised_deadline=excluded.revised_deadline,"
                 " reason=excluded.reason");
    st.bind(1, static_cast<std::int64_t>(e.assignmentId))
        .bind(2, static_cast<std::int64_t>(e.studentId))
        .bind(3, static_cast<std::int64_t>(e.grantedBy))
        .bind(4, e.revisedDeadline.toString())
        .bind(5, e.reason);
    st.exec();
}

std::optional<Extension> SqliteExtensionRepository::find(Id assignmentId, Id studentId) {
    Statement st(db_,
                 "SELECT extension_id, assignment_id, student_id, granted_by, revised_deadline, reason"
                 " FROM Extension WHERE assignment_id=?1 AND student_id=?2");
    st.bind(1, static_cast<std::int64_t>(assignmentId)).bind(2, static_cast<std::int64_t>(studentId));
    if (!st.step()) return std::nullopt;
    Extension e;
    e.id = st.integer(0);
    e.assignmentId = st.integer(1);
    e.studentId = st.integer(2);
    e.grantedBy = st.integer(3);
    e.revisedDeadline = DateTime::parse(st.text(4));
    e.reason = st.text(5);
    return e;
}

// ---- Submissions ---------------------------------------------------------------------------
Id SqliteSubmissionRepository::add(const Submission& s) {
    Statement st(
        db_,
        "INSERT INTO Submission (assignment_id, student_id, submitted_at, file_path, status, mark, feedback)"
        " VALUES (?1,?2,?3,?4,?5,?6,?7)");
    st.bind(1, static_cast<std::int64_t>(s.assignmentId))
        .bind(2, static_cast<std::int64_t>(s.studentId))
        .bind(3, s.submittedAt.toString())
        .bind(4, s.filePath)
        .bind(5, toString(s.status))
        .bind(6, s.mark)
        .bind(7, s.feedback);
    st.exec();
    return sqlite3_last_insert_rowid(db_.handle());
}

void SqliteSubmissionRepository::update(const Submission& s) {
    Statement st(db_,
                 "UPDATE Submission SET submitted_at=?1, file_path=?2, status=?3, mark=?4, feedback=?5"
                 " WHERE submission_id=?6");
    st.bind(1, s.submittedAt.toString())
        .bind(2, s.filePath)
        .bind(3, toString(s.status))
        .bind(4, s.mark)
        .bind(5, s.feedback)
        .bind(6, static_cast<std::int64_t>(s.id));
    st.exec();
}

std::optional<Submission> SqliteSubmissionRepository::findById(Id id) {
    Statement st(db_,
                 std::string("SELECT ") + kSubmissionCols + " FROM Submission s WHERE s.submission_id=?1");
    st.bind(1, static_cast<std::int64_t>(id));
    if (st.step()) return submissionFromRow(st);
    return std::nullopt;
}

std::optional<Submission> SqliteSubmissionRepository::find(Id assignmentId, Id studentId) {
    Statement st(db_, std::string("SELECT ") + kSubmissionCols +
                          " FROM Submission s WHERE s.assignment_id=?1 AND s.student_id=?2");
    st.bind(1, static_cast<std::int64_t>(assignmentId)).bind(2, static_cast<std::int64_t>(studentId));
    if (st.step()) return submissionFromRow(st);
    return std::nullopt;
}

std::vector<Submission> SqliteSubmissionRepository::listByAssignment(Id assignmentId) {
    Statement st(db_, std::string("SELECT ") + kSubmissionCols +
                          " FROM Submission s WHERE s.assignment_id=?1 ORDER BY s.student_id");
    st.bind(1, static_cast<std::int64_t>(assignmentId));
    std::vector<Submission> out;
    while (st.step()) out.push_back(submissionFromRow(st));
    return out;
}

std::vector<Submission> SqliteSubmissionRepository::listByStudent(Id studentId) {
    Statement st(db_, std::string("SELECT ") + kSubmissionCols +
                          " FROM Submission s WHERE s.student_id=?1 ORDER BY s.submission_id");
    st.bind(1, static_cast<std::int64_t>(studentId));
    std::vector<Submission> out;
    while (st.step()) out.push_back(submissionFromRow(st));
    return out;
}

std::vector<Submission> SqliteSubmissionRepository::listMarkedByDepartment(Id departmentId) {
    Statement st(db_, std::string("SELECT ") + kSubmissionCols +
                          " FROM Submission s JOIN Assignment a ON a.assignment_id=s.assignment_id"
                          " JOIN Course c ON c.course_id=a.course_id"
                          " WHERE c.department_id=?1 AND s.status IN ('Marked','Moderated')"
                          " ORDER BY s.submission_id");
    st.bind(1, static_cast<std::int64_t>(departmentId));
    std::vector<Submission> out;
    while (st.step()) out.push_back(submissionFromRow(st));
    return out;
}

// ---- Mark audit ----------------------------------------------------------------------------
void SqliteMarkAuditRepository::add(const MarkAudit& a) {
    Statement st(db_,
                 "INSERT INTO MarkAudit (submission_id, changed_by, old_mark, new_mark, reason, changed_at)"
                 " VALUES (?1,?2,?3,?4,?5,?6)");
    st.bind(1, static_cast<std::int64_t>(a.submissionId))
        .bind(2, static_cast<std::int64_t>(a.changedBy))
        .bind(3, a.oldMark)
        .bind(4, a.newMark)
        .bind(5, a.reason)
        .bind(6, a.changedAt.toString());
    st.exec();
}

std::vector<MarkAudit> SqliteMarkAuditRepository::listBySubmission(Id submissionId) {
    Statement st(db_,
                 "SELECT audit_id, submission_id, changed_by, old_mark, new_mark, reason, changed_at"
                 " FROM MarkAudit WHERE submission_id=?1 ORDER BY audit_id");
    st.bind(1, static_cast<std::int64_t>(submissionId));
    std::vector<MarkAudit> out;
    while (st.step()) {
        MarkAudit a;
        a.id = st.integer(0);
        a.submissionId = st.integer(1);
        a.changedBy = st.integer(2);
        a.oldMark = st.optReal(3);
        a.newMark = st.real(4);
        a.reason = st.text(5);
        a.changedAt = DateTime::parse(st.text(6));
        out.push_back(a);
    }
    return out;
}

// ---- Moderation ----------------------------------------------------------------------------
namespace {
Moderation moderationFromRow(const Statement& s) {
    Moderation m;
    m.id = s.integer(0);
    m.submissionId = s.integer(1);
    m.moderatorId = s.integer(2);
    m.outcome = outcomeFromString(s.text(3));
    m.comments = s.text(4);
    m.moderatedAt = DateTime::parse(s.text(5));
    return m;
}
constexpr const char* kModCols =
    "moderation_id, submission_id, moderator_id, outcome, comments, moderated_at";
}  // namespace

void SqliteModerationRepository::add(const Moderation& m) {
    Statement st(db_,
                 "INSERT INTO Moderation (submission_id, moderator_id, outcome, comments, moderated_at)"
                 " VALUES (?1,?2,?3,?4,?5)");
    st.bind(1, static_cast<std::int64_t>(m.submissionId))
        .bind(2, static_cast<std::int64_t>(m.moderatorId))
        .bind(3, toString(m.outcome))
        .bind(4, m.comments)
        .bind(5, m.moderatedAt.toString());
    st.exec();
}

std::optional<Moderation> SqliteModerationRepository::latest(Id submissionId) {
    Statement st(db_, std::string("SELECT ") + kModCols +
                          " FROM Moderation WHERE submission_id=?1 ORDER BY moderation_id DESC LIMIT 1");
    st.bind(1, static_cast<std::int64_t>(submissionId));
    if (st.step()) return moderationFromRow(st);
    return std::nullopt;
}

std::vector<Moderation> SqliteModerationRepository::listBySubmission(Id submissionId) {
    Statement st(db_, std::string("SELECT ") + kModCols +
                          " FROM Moderation WHERE submission_id=?1 ORDER BY moderation_id");
    st.bind(1, static_cast<std::int64_t>(submissionId));
    std::vector<Moderation> out;
    while (st.step()) out.push_back(moderationFromRow(st));
    return out;
}

}  // namespace lms
