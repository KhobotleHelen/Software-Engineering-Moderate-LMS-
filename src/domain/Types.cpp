#include "domain/Types.h"

#include "common/Errors.h"

namespace lms {

std::string toString(Role r) {
    switch (r) {
        case Role::Student:
            return "Student";
        case Role::Lecturer:
            return "Lecturer";
        case Role::HoD:
            return "HoD";
        case Role::Administrator:
            return "Administrator";
    }
    throw LmsError("Unknown role");
}

std::string toString(SubmissionStatus s) {
    switch (s) {
        case SubmissionStatus::Submitted:
            return "Submitted";
        case SubmissionStatus::Late:
            return "Late";
        case SubmissionStatus::Marked:
            return "Marked";
        case SubmissionStatus::Moderated:
            return "Moderated";
        case SubmissionStatus::Released:
            return "Released";
    }
    throw LmsError("Unknown status");
}

std::string toString(ModerationOutcome o) {
    return o == ModerationOutcome::Approved ? "Approved" : "CorrectionRequired";
}

Role roleFromString(const std::string& s) {
    if (s == "Student") return Role::Student;
    if (s == "Lecturer") return Role::Lecturer;
    if (s == "HoD") return Role::HoD;
    if (s == "Administrator") return Role::Administrator;
    throw ValidationError("Unknown role: " + s);
}

SubmissionStatus statusFromString(const std::string& s) {
    if (s == "Submitted") return SubmissionStatus::Submitted;
    if (s == "Late") return SubmissionStatus::Late;
    if (s == "Marked") return SubmissionStatus::Marked;
    if (s == "Moderated") return SubmissionStatus::Moderated;
    if (s == "Released") return SubmissionStatus::Released;
    throw LmsError("Unknown submission status in database: " + s);
}

ModerationOutcome outcomeFromString(const std::string& s) {
    if (s == "Approved") return ModerationOutcome::Approved;
    if (s == "CorrectionRequired") return ModerationOutcome::CorrectionRequired;
    throw LmsError("Unknown moderation outcome in database: " + s);
}

}  // namespace lms
