#include "services/AssessmentService.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <sstream>

#include "common/Errors.h"
#include "common/Validation.h"

namespace lms {
namespace {

std::string fmt(double v) {
    std::ostringstream o;
    o.setf(std::ios::fixed);
    o.precision(2);
    o << v;
    return o.str();
}

}  // namespace

Submission AssessmentService::loadSubmission(Id id) {
    const auto s = submissions_.findById(id);
    if (!s) throw NotFoundError("Submission not found.");
    return *s;
}

Assignment AssessmentService::assignmentOf(const Submission& s) {
    const auto a = assignments_.findById(s.assignmentId);
    if (!a) throw NotFoundError("Assignment not found.");
    return *a;
}

SubmissionView AssessmentService::toView(const Submission& s) {
    SubmissionView v;
    v.submission = s;
    const Assignment a = assignmentOf(s);
    v.assignmentTitle = a.title;
    v.maxMark = a.maxMark;
    if (const auto c = courses_.findById(a.courseId)) v.courseName = c->name;
    if (const auto u = users_.findById(s.studentId)) v.studentUsername = u->username();
    v.lateDays = penalty_.lateDays(deadlines_.effectiveDeadline(a, s.studentId), s.submittedAt);
    v.latestModeration = moderation_.latest(s.id);
    return v;
}

MarkResult AssessmentService::recordMark(const User& lecturer, Id submissionId, double rawMark,
                                         const std::string& feedback, const std::string& reason) {
    access_.require(lecturer, Permission::MarkWork);
    Submission s = loadSubmission(submissionId);
    const Assignment a = assignmentOf(s);
    access_.requireTeaches(lecturer, a.courseId);

    if (s.status == SubmissionStatus::Moderated || s.status == SubmissionStatus::Released)
        throw BusinessRuleError(
            "This mark has already been approved by the Head of Department and can no longer be changed.");
    validation::requireMark(rawMark, a.maxMark);
    validation::requireNonEmpty(feedback, "Feedback", 4000);

    MarkResult r;
    r.rawMark = rawMark;
    r.lateDays = penalty_.lateDays(deadlines_.effectiveDeadline(a, s.studentId), s.submittedAt);
    r.finalMark = penalty_.apply(rawMark, a.maxMark, r.lateDays);
    r.penalty = rawMark - r.finalMark;

    const std::optional<double> oldMark = s.mark;
    const bool changed = !oldMark || std::fabs(*oldMark - r.finalMark) > 1e-9;
    const std::string userReason = validation::trim(reason);
    if (oldMark && changed && userReason.empty())
        throw ValidationError("A reason is required when an existing mark is changed.");

    std::string auditReason = userReason.empty() ? "Initial mark" : userReason;
    auditReason += " [raw mark " + fmt(rawMark) + "/" + fmt(a.maxMark);
    if (r.lateDays > 0)
        auditReason += ", " + std::to_string(r.lateDays) + " day(s) late, penalty " + fmt(r.penalty);
    auditReason += "]";

    tx_.run([&] {
        s.mark = r.finalMark;
        s.feedback = feedback;
        s.status = SubmissionStatus::Marked;
        submissions_.update(s);
        if (changed) {
            MarkAudit m;
            m.submissionId = s.id;
            m.changedBy = lecturer.id();
            m.oldMark = oldMark;
            m.newMark = r.finalMark;
            m.reason = auditReason;
            m.changedAt = clock_.now();
            audit_.add(m);  // same transaction as the update (Phase 3 schema note)
        }
    });
    return r;
}

std::vector<SubmissionView> AssessmentService::submissionsForAssignment(const User& lecturer,
                                                                        Id assignmentId) {
    access_.require(lecturer, Permission::MarkWork);
    const auto a = assignments_.findById(assignmentId);
    if (!a) throw NotFoundError("Assignment not found.");
    access_.requireTeaches(lecturer, a->courseId);
    const auto subs = submissions_.listByAssignment(assignmentId);
    std::vector<SubmissionView> out;
    out.reserve(subs.size());
    std::transform(subs.begin(), subs.end(), std::back_inserter(out),
                   [this](const Submission& s) { return toView(s); });
    return out;
}

std::vector<SubmissionView> AssessmentService::awaitingModeration(const User& hod) {
    access_.require(hod, Permission::ModerateMarks);
    const auto dept = access_.departmentOfHead(hod);
    if (!dept) throw BusinessRuleError("You have not been assigned as head of a department yet.");
    const auto subs = submissions_.listMarkedByDepartment(dept->id);
    std::vector<SubmissionView> out;
    out.reserve(subs.size());
    std::transform(subs.begin(), subs.end(), std::back_inserter(out),
                   [this](const Submission& s) { return toView(s); });
    return out;
}

void AssessmentService::moderate(const User& hod, Id submissionId, ModerationOutcome outcome,
                                 const std::string& comments) {
    access_.require(hod, Permission::ModerateMarks);
    Submission s = loadSubmission(submissionId);
    access_.requireHeadOfCourseDepartment(hod, assignmentOf(s).courseId);
    if (s.status != SubmissionStatus::Marked)
        throw BusinessRuleError("Only marked work awaiting review can be moderated (current status: " +
                                toString(s.status) + ").");
    if (outcome == ModerationOutcome::CorrectionRequired)
        validation::requireNonEmpty(comments, "Comments explaining the correction", 1000);
    tx_.run([&] {
        Moderation m;
        m.submissionId = s.id;
        m.moderatorId = hod.id();
        m.outcome = outcome;
        m.comments = validation::trim(comments);
        m.moderatedAt = clock_.now();
        moderation_.add(m);
        if (outcome == ModerationOutcome::Approved) {
            s.status = SubmissionStatus::Moderated;
            submissions_.update(s);
        }
    });
}

void AssessmentService::release(const User& hod, Id submissionId) {
    access_.require(hod, Permission::ReleaseMarks);
    Submission s = loadSubmission(submissionId);
    access_.requireHeadOfCourseDepartment(hod, assignmentOf(s).courseId);
    const auto latest = moderation_.latest(s.id);
    if (s.status != SubmissionStatus::Moderated || !latest || latest->outcome != ModerationOutcome::Approved)
        throw BusinessRuleError("A mark can only be released after the Head of Department has approved it.");
    tx_.run([&] {
        s.status = SubmissionStatus::Released;
        submissions_.update(s);
    });
}

int AssessmentService::releaseAllApproved(const User& hod, Id courseId) {
    access_.require(hod, Permission::ReleaseMarks);
    access_.requireHeadOfCourseDepartment(hod, courseId);
    int count = 0;
    tx_.run([&] {
        for (const auto& a : assignments_.listByCourse(courseId)) {
            for (const auto& s : submissions_.listByAssignment(a.id)) {
                if (s.status == SubmissionStatus::Moderated) {
                    release(hod, s.id);
                    ++count;
                }
            }
        }
    });
    return count;
}

std::vector<SubmissionView> AssessmentService::resultsForStudent(const User& student) {
    access_.require(student, Permission::ViewOwnMarks);
    std::vector<SubmissionView> out;
    for (const auto& s : submissions_.listByStudent(student.id())) {
        SubmissionView v = toView(s);
        v.latestModeration.reset();  // internal moderation notes are not shown to students
        if (v.submission.status != SubmissionStatus::Released) {
            v.submission.mark.reset();  // BR: nothing is visible before HoD release
            v.submission.feedback.clear();
        }
        out.push_back(std::move(v));
    }
    return out;
}

std::vector<AuditEntry> AssessmentService::auditTrail(const User& actor, Id submissionId) {
    access_.require(actor, Permission::ViewAuditLog);
    const Submission s = loadSubmission(submissionId);
    access_.requireCourseStaff(actor, assignmentOf(s).courseId);
    std::vector<AuditEntry> out;
    for (const auto& m : audit_.listBySubmission(submissionId)) {
        AuditEntry e;
        e.audit = m;
        if (const auto u = users_.findById(m.changedBy)) e.changedByUsername = u->username();
        out.push_back(std::move(e));
    }
    return out;
}

}  // namespace lms
