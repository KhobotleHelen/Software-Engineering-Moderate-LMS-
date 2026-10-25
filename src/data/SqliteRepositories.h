// SQLite implementations of the repository interfaces. All queries are parameterised.
#pragma once
#include "data/Database.h"
#include "data/Repositories.h"

namespace lms {

class SqliteUserRepository final : public IUserRepository {
public:
    explicit SqliteUserRepository(Database& db) : db_(db) {}
    Id add(const User& user) override;
    void update(const User& user) override;
    std::unique_ptr<User> findById(Id id) override;
    std::unique_ptr<User> findByUsername(const std::string& username) override;
    std::vector<std::unique_ptr<User>> listAll() override;
    std::int64_t count() override;

private:
    Database& db_;
};

class SqliteDepartmentRepository final : public IDepartmentRepository {
public:
    explicit SqliteDepartmentRepository(Database& db) : db_(db) {}
    Id add(const std::string& name) override;
    std::optional<Department> findById(Id id) override;
    std::vector<Department> listAll() override;
    std::optional<Department> findByHead(Id hodId) override;
    void setHead(Id departmentId, Id hodId) override;

private:
    Database& db_;
};

class SqliteCourseRepository final : public ICourseRepository {
public:
    explicit SqliteCourseRepository(Database& db) : db_(db) {}
    Id add(Id departmentId, const std::string& name) override;
    std::optional<Course> findById(Id id) override;
    std::vector<Course> listAll() override;
    std::vector<Course> listByDepartment(Id departmentId) override;
    std::vector<Course> listByLecturer(Id lecturerId) override;
    std::vector<Course> listByStudent(Id studentId) override;
    void addLecturer(Id courseId, Id lecturerId) override;
    bool isLecturerOf(Id courseId, Id lecturerId) override;

private:
    Database& db_;
};

class SqliteEnrolmentRepository final : public IEnrolmentRepository {
public:
    explicit SqliteEnrolmentRepository(Database& db) : db_(db) {}
    Id add(Id studentId, Id courseId, const DateTime& at) override;
    bool exists(Id studentId, Id courseId) override;
    std::vector<Id> studentIdsOfCourse(Id courseId) override;

private:
    Database& db_;
};

class SqliteAssignmentRepository final : public IAssignmentRepository {
public:
    explicit SqliteAssignmentRepository(Database& db) : db_(db) {}
    Id add(const Assignment& a) override;
    std::optional<Assignment> findById(Id id) override;
    std::vector<Assignment> listByCourse(Id courseId) override;
    std::vector<Assignment> listForStudent(Id studentId) override;

private:
    Database& db_;
};

class SqliteExtensionRepository final : public IExtensionRepository {
public:
    explicit SqliteExtensionRepository(Database& db) : db_(db) {}
    void upsert(const Extension& e) override;
    std::optional<Extension> find(Id assignmentId, Id studentId) override;

private:
    Database& db_;
};

class SqliteSubmissionRepository final : public ISubmissionRepository {
public:
    explicit SqliteSubmissionRepository(Database& db) : db_(db) {}
    Id add(const Submission& s) override;
    void update(const Submission& s) override;
    std::optional<Submission> findById(Id id) override;
    std::optional<Submission> find(Id assignmentId, Id studentId) override;
    std::vector<Submission> listByAssignment(Id assignmentId) override;
    std::vector<Submission> listByStudent(Id studentId) override;
    std::vector<Submission> listMarkedByDepartment(Id departmentId) override;

private:
    Database& db_;
};

class SqliteMarkAuditRepository final : public IMarkAuditRepository {
public:
    explicit SqliteMarkAuditRepository(Database& db) : db_(db) {}
    void add(const MarkAudit& a) override;
    std::vector<MarkAudit> listBySubmission(Id submissionId) override;

private:
    Database& db_;
};

class SqliteModerationRepository final : public IModerationRepository {
public:
    explicit SqliteModerationRepository(Database& db) : db_(db) {}
    void add(const Moderation& m) override;
    std::optional<Moderation> latest(Id submissionId) override;
    std::vector<Moderation> listBySubmission(Id submissionId) override;

private:
    Database& db_;
};

}  // namespace lms
