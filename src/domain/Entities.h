// Plain domain entities (Phase 3 class diagram). Behaviour lives in services.
#pragma once
#include <optional>
#include <string>

#include "common/DateTime.h"
#include "domain/Types.h"

namespace lms {

struct Department {
    Id id = 0;
    std::string name;
    std::optional<Id> headId;  // Added during implementation: links an HoD to a department (see docs)
};

struct Course {
    Id id = 0;
    Id departmentId = 0;
    std::string name;
};

struct Enrolment {
    Id id = 0;
    Id studentId = 0;
    Id courseId = 0;
    DateTime enrolledAt;
};

struct Assignment {
    Id id = 0;
    Id courseId = 0;
    Id createdBy = 0;
    std::string title;
    std::string description;
    DateTime deadline;
    double maxMark = 0.0;
};

struct Extension {
    Id id = 0;
    Id assignmentId = 0;
    Id studentId = 0;
    Id grantedBy = 0;
    DateTime revisedDeadline;
    std::string reason;
};

struct Submission {
    Id id = 0;
    Id assignmentId = 0;
    Id studentId = 0;
    DateTime submittedAt;
    std::string filePath;
    SubmissionStatus status = SubmissionStatus::Submitted;
    std::optional<double> mark;
    std::string feedback;
};

struct MarkAudit {
    Id id = 0;
    Id submissionId = 0;
    Id changedBy = 0;
    std::optional<double> oldMark;
    double newMark = 0.0;
    std::string reason;
    DateTime changedAt;
};

struct Moderation {
    Id id = 0;
    Id submissionId = 0;
    Id moderatorId = 0;
    ModerationOutcome outcome = ModerationOutcome::Approved;
    std::string comments;
    DateTime moderatedAt;
};

}  // namespace lms
