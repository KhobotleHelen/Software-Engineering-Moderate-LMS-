// Repository interfaces (Repository/DAO pattern, ISP, DIP). Services depend on these, never on SQLite.
#pragma once
#include <memory>
#include <optional>
#include <vector>

#include "domain/Entities.h"
#include "domain/User.h"

namespace lms {

class IUserRepository {
public:
    virtual ~IUserRepository() = default;
    virtual Id add(const User& user) = 0;  // returns new id; throws ValidationError if username exists
    virtual void update(const User& user) = 0;
    virtual std::unique_ptr<User> findById(Id id) = 0;
    virtual std::unique_ptr<User> findByUsername(const std::string& username) = 0;
    virtual std::vector<std::unique_ptr<User>> listAll() = 0;
    virtual std::int64_t count() = 0;
};

class IDepartmentRepository {
public:
    virtual ~IDepartmentRepository() = default;
    virtual Id add(const std::string& name) = 0;
    virtual std::optional<Department> findById(Id id) = 0;
    virtual std::vector<Department> listAll() = 0;
    virtual std::optional<Department> findByHead(Id hodId) = 0;
    virtual void setHead(Id departmentId, Id hodId) = 0;
};

class ICourseRepository {
public:
    virtual ~ICourseRepository() = default;
    virtual Id add(Id departmentId, const std::string& name) = 0;
    virtual std::optional<Course> findById(Id id) = 0;
    virtual std::vector<Course> listAll() = 0;
    virtual std::vector<Course> listByDepartment(Id departmentId) = 0;
    virtual std::vector<Course> listByLecturer(Id lecturerId) = 0;
    virtual std::vector<Course> listByStudent(Id studentId) = 0;
    virtual void addLecturer(Id courseId, Id lecturerId) = 0;
    virtual bool isLecturerOf(Id courseId, Id lecturerId) = 0;
};

class IEnrolmentRepository {
public:
    virtual ~IEnrolmentRepository() = default;
    virtual Id add(Id studentId, Id courseId, const DateTime& at) = 0;
    virtual bool exists(Id studentId, Id courseId) = 0;
    virtual std::vector<Id> studentIdsOfCourse(Id courseId) = 0;
};

class IAssignmentRepository {
public:
    virtual ~IAssignmentRepository() = default;
    virtual Id add(const Assignment& a) = 0;
    virtual std::optional<Assignment> findById(Id id) = 0;
    virtual std::vector<Assignment> listByCourse(Id courseId) = 0;
    virtual std::vector<Assignment> listForStudent(Id studentId) = 0;
};

class IExtensionRepository {
public:
    virtual ~IExtensionRepository() = default;
    virtual void upsert(const Extension& e) = 0;  // one extension per (assignment, student)
    virtual std::optional<Extension> find(Id assignmentId, Id studentId) = 0;
};

class ISubmissionRepository {
public:
    virtual ~ISubmissionRepository() = default;
    virtual Id add(const Submission& s) = 0;
    virtual void update(const Submission& s) = 0;
    virtual std::optional<Submission> findById(Id id) = 0;
    virtual std::optional<Submission> find(Id assignmentId, Id studentId) = 0;
    virtual std::vector<Submission> listByAssignment(Id assignmentId) = 0;
    virtual std::vector<Submission> listByStudent(Id studentId) = 0;
    virtual std::vector<Submission> listMarkedByDepartment(Id departmentId) = 0;  // status Marked/Moderated
};

class IMarkAuditRepository {
public:
    virtual ~IMarkAuditRepository() = default;
    virtual void add(const MarkAudit& a) = 0;
    virtual std::vector<MarkAudit> listBySubmission(Id submissionId) = 0;
};

class IModerationRepository {
public:
    virtual ~IModerationRepository() = default;
    virtual void add(const Moderation& m) = 0;
    virtual std::optional<Moderation> latest(Id submissionId) = 0;
    virtual std::vector<Moderation> listBySubmission(Id submissionId) = 0;
};

}  // namespace lms
