// Core enumerations and their string conversions (used for the SQL CHECK-constrained columns).
#pragma once
#include <cstdint>
#include <string>

namespace lms {

using Id = std::int64_t;

enum class Role { Student, Lecturer, HoD, Administrator };
enum class SubmissionStatus { Submitted, Late, Marked, Moderated, Released };
enum class ModerationOutcome { Approved, CorrectionRequired };

// One permission per protected business operation (FR-02, NFR-05).
enum class Permission {
    ManageUsers,         // FR-03
    ManageAcademicData,  // FR-04
    CreateAssignment,    // FR-05
    ViewAssignments,     // FR-06
    SubmitWork,          // FR-07, FR-08
    GrantExtension,      // FR-09
    MarkWork,            // FR-11
    ModerateMarks,       // FR-12
    ReleaseMarks,        // FR-12
    ViewOwnMarks,        // FR-12, FR-14
    ViewAuditLog         // FR-13
};

std::string toString(Role r);
std::string toString(SubmissionStatus s);
std::string toString(ModerationOutcome o);
Role roleFromString(const std::string& s);
SubmissionStatus statusFromString(const std::string& s);
ModerationOutcome outcomeFromString(const std::string& s);

}  // namespace lms
