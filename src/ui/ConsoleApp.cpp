#include "ui/ConsoleApp.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "common/Validation.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

namespace lms {
namespace {

void logError(const std::string& detail) {
    std::ofstream f("lms_error.log", std::ios::app);
    if (f) f << detail << '\n';
}

// Turns terminal echo off while a password is typed (only when reading from a real terminal).
class EchoGuard {
public:
    explicit EchoGuard(bool active) : active_(active) {
        if (!active_) return;
#ifdef _WIN32
        h_ = GetStdHandle(STD_INPUT_HANDLE);
        GetConsoleMode(h_, &mode_);
        SetConsoleMode(h_, mode_ & ~ENABLE_ECHO_INPUT);
#else
        tcgetattr(STDIN_FILENO, &old_);
        termios t = old_;
        t.c_lflag &= ~static_cast<tcflag_t>(ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &t);
#endif
    }
    ~EchoGuard() {
        if (!active_) return;
#ifdef _WIN32
        SetConsoleMode(h_, mode_);
#else
        tcsetattr(STDIN_FILENO, TCSANOW, &old_);
#endif
    }
    EchoGuard(const EchoGuard&) = delete;
    EchoGuard& operator=(const EchoGuard&) = delete;

private:
    bool active_;
#ifdef _WIN32
    HANDLE h_{};
    DWORD mode_{};
#else
    termios old_{};
#endif
};

bool isTerminal() {
#ifdef _WIN32
    return true;
#else
    return isatty(STDIN_FILENO) != 0;
#endif
}

}  // namespace

ConsoleApp::ConsoleApp(AppContext& ctx, std::istream& in, std::ostream& out, const IClock& clock)
    : ctx_(ctx), in_(in), out_(out), clock_(clock) {}

// ---- input helpers -------------------------------------------------------------------------
std::string ConsoleApp::ask(const std::string& prompt) {
    out_ << prompt << ": " << std::flush;
    std::string line;
    if (!std::getline(in_, line)) throw InputClosed();
    return validation::trim(line);
}

std::string ConsoleApp::askSecret(const std::string& prompt) {
    out_ << prompt << ": " << std::flush;
    std::string line;
    {
        EchoGuard guard(&in_ == &std::cin && isTerminal());
        if (!std::getline(in_, line)) throw InputClosed();
    }
    if (&in_ == &std::cin && isTerminal()) out_ << '\n';
    if (!line.empty() && line.back() == '\r') line.pop_back();
    return line;  // not trimmed: spaces may be part of a password
}

Id ConsoleApp::askId(const std::string& prompt) {
    const std::string s = ask(prompt);
    if (s.empty() || s.size() > 18 || s.find_first_not_of("0123456789") != std::string::npos)
        throw ValidationError("Please enter a valid number.");
    return std::stoll(s);
}

double ConsoleApp::askNumber(const std::string& prompt) {
    const std::string s = ask(prompt);
    char* end = nullptr;
    const double v = std::strtod(s.c_str(), &end);
    if (s.empty() || end == s.c_str() || *end != '\0') throw ValidationError("Please enter a valid number.");
    return v;
}

DateTime ConsoleApp::askDateTime(const std::string& prompt) {
    return DateTime::parse(ask(prompt + " (YYYY-MM-DD HH:MM)"));
}

bool ConsoleApp::askYesNo(const std::string& prompt) {
    const std::string s = ask(prompt + " (y/n)");
    return s == "y" || s == "Y" || s == "yes";
}

int ConsoleApp::askMenuChoice(const std::vector<std::string>& items, const std::string& title) {
    out_ << "\n=== " << title << " ===\n";
    for (std::size_t i = 0; i < items.size(); ++i) out_ << "  " << (i + 1) << ". " << items[i] << '\n';
    out_ << "  0. Log out\n";
    const std::string s = ask("Choose an option");
    if (s.size() == 1 && s[0] == '0') return 0;
    if (s.empty() || s.size() > 2 || s.find_first_not_of("0123456789") != std::string::npos) return -1;
    const int n = std::stoi(s);
    return (n >= 1 && n <= static_cast<int>(items.size())) ? n : -1;
}

std::string ConsoleApp::money(double v) {
    std::ostringstream o;
    o << std::fixed << std::setprecision(2) << v;
    return o.str();
}

// ---- top level ----------------------------------------------------------------------------
void ConsoleApp::run() {
    out_ << "Moderated Learning Management System\n";
    try {
        if (ctx_.users.count() == 0) firstRunSetup();
        for (;;) {
            auto user = loginScreen();
            if (!user) break;
            roleMenu(*user);
            out_ << "You have been logged out.\n";
        }
    } catch (const InputClosed&) {
        out_ << "\nInput closed. Goodbye.\n";
    }
}

void ConsoleApp::firstRunSetup() {
    out_ << "\nFirst-time setup: create the administrator account.\n";
    for (;;) {
        try {
            const std::string u = ask("Administrator username");
            const std::string p = askSecret("Password (at least 8 characters)");
            if (askSecret("Repeat password") != p) throw ValidationError("The passwords do not match.");
            ctx_.userService.createInitialAdministrator(u, p);
            out_ << "Administrator account created.\n";
            return;
        } catch (const LmsError& e) {
            out_ << "Error: " << e.what() << '\n';
        }
    }
}

std::unique_ptr<User> ConsoleApp::loginScreen() {
    for (;;) {
        out_ << "\n--- Log in (leave the username empty to exit) ---\n";
        const std::string username = ask("Username");
        if (username.empty()) return nullptr;
        const std::string password = askSecret("Password");
        try {
            auto user = ctx_.auth.login(username, password);
            out_ << "Welcome, " << user->username() << " (" << user->roleName() << ").\n";
            return user;
        } catch (const LmsError& e) {
            out_ << "Error: " << e.what() << '\n';
        } catch (const std::exception& e) {
            logError(std::string("login: ") + e.what());
            out_ << "Unexpected error. Please try again.\n";
        }
    }
}

void ConsoleApp::roleMenu(User& user) {
    switch (user.role()) {  // polymorphic dispatch on the abstract User
        case Role::Student:
            return studentMenu(user);
        case Role::Lecturer:
            return lecturerMenu(user);
        case Role::HoD:
            return hodMenu(user);
        case Role::Administrator:
            return adminMenu(user);
    }
}

namespace {
// Runs one menu action, converting every exception into a friendly message (NFR-06: never crash).
template <typename F>
void guarded(std::ostream& out, F&& action) {
    try {
        action();
    } catch (const InputClosed&) {
        throw;
    } catch (const DatabaseError& e) {
        logError(std::string("database: ") + e.what());
        out << "A database error occurred. Your change was not saved. Please try again or contact the "
               "administrator.\n";
    } catch (const LmsError& e) {
        out << "Error: " << e.what() << '\n';
    } catch (const std::exception& e) {
        logError(std::string("unexpected: ") + e.what());
        out << "An unexpected error occurred. Please try again or contact the administrator.\n";
    }
}
}  // namespace

void ConsoleApp::changePassword(User& user) {
    const std::string oldPw = askSecret("Current password");
    const std::string newPw = askSecret("New password (at least 8 characters)");
    if (askSecret("Repeat new password") != newPw) throw ValidationError("The passwords do not match.");
    ctx_.auth.changePassword(user, oldPw, newPw);
    out_ << "Password changed.\n";
}

// ---- student -------------------------------------------------------------------------------
void ConsoleApp::studentMenu(User& user) {
    const std::vector<std::string> items = {"My courses", "My assignments", "Submit or resubmit work",
                                            "My released marks and feedback", "Change my password"};
    for (;;) {
        const int c = askMenuChoice(items, "Student menu");
        if (c == 0) return;
        guarded(out_, [&] {
            switch (c) {
                case 1:
                    showCourses(user);
                    break;
                case 2:
                    showStudentAssignments(user);
                    break;
                case 3:
                    submitWork(user);
                    break;
                case 4:
                    showStudentResults(user);
                    break;
                case 5:
                    changePassword(user);
                    break;
                default:
                    out_ << "Please choose a number from the menu.\n";
            }
        });
    }
}

void ConsoleApp::showCourses(const User& user) {
    const auto courses = ctx_.academic.listCourses(user);
    if (courses.empty()) out_ << "No courses found.\n";
    for (const auto& c : courses) out_ << "  [" << c.id << "] " << c.name << '\n';
}

void ConsoleApp::showStudentAssignments(const User& user) {
    const auto list = ctx_.assignments.listForStudent(user);
    if (list.empty()) out_ << "You have no assignments.\n";
    for (const auto& v : list) {
        out_ << "  [" << v.assignment.id << "] " << v.assignment.title << " | due "
             << v.assignment.deadline.toString() << " | max " << money(v.assignment.maxMark) << " | "
             << (v.submissionStatus ? "submitted (" + toString(*v.submissionStatus) + ")" : "not submitted")
             << '\n';
    }
}

void ConsoleApp::submitWork(const User& user) {
    showStudentAssignments(user);
    const Id id = askId("Assignment number to submit (or resubmit)");
    out_ << "Your deadline for this assignment: " << ctx_.submissions.effectiveDeadline(user, id).toString()
         << '\n';
    const std::string path = ask("Path of the file to submit");
    if (!std::filesystem::exists(path)) {
        out_ << "Warning: that file was not found on this computer.\n";
        if (!askYesNo("Submit this path anyway?")) {
            out_ << "Cancelled.\n";
            return;
        }
    }
    if (!askYesNo("Confirm submission")) {
        out_ << "Cancelled.\n";
        return;
    }
    out_ << ctx_.submissions.submit(user, id, path).message << '\n';
}

void ConsoleApp::showStudentResults(const User& user) {
    const auto list = ctx_.assessment.resultsForStudent(user);
    if (list.empty()) out_ << "You have not submitted any work yet.\n";
    for (const auto& v : list) {
        out_ << "  [" << v.submission.id << "] " << v.courseName << " - " << v.assignmentTitle << ": ";
        if (v.submission.status == SubmissionStatus::Released) {
            out_ << money(*v.submission.mark) << " / " << money(v.maxMark)
                 << "\n      Feedback: " << v.submission.feedback << '\n';
        } else {
            out_ << "not released yet\n";
        }
    }
}

// ---- lecturer ------------------------------------------------------------------------------
void ConsoleApp::lecturerMenu(User& user) {
    const std::vector<std::string> items = {"My courses",
                                            "Create an assignment",
                                            "List assignments of a course",
                                            "Grant a student an extension",
                                            "View submissions for an assignment",
                                            "Mark a submission",
                                            "View mark history of a submission",
                                            "Change my password"};
    for (;;) {
        const int c = askMenuChoice(items, "Lecturer menu");
        if (c == 0) return;
        guarded(out_, [&] {
            switch (c) {
                case 1:
                    showCourses(user);
                    break;
                case 2:
                    createAssignment(user);
                    break;
                case 3:
                    listAssignments(user);
                    break;
                case 4:
                    grantExtension(user);
                    break;
                case 5:
                    viewSubmissions(user);
                    break;
                case 6:
                    markSubmission(user);
                    break;
                case 7:
                    showAudit(user);
                    break;
                case 8:
                    changePassword(user);
                    break;
                default:
                    out_ << "Please choose a number from the menu.\n";
            }
        });
    }
}

void ConsoleApp::createAssignment(const User& user) {
    showCourses(user);
    const Id course = askId("Course number");
    const std::string title = ask("Title");
    const std::string text = ask("Instructions");
    const DateTime deadline = askDateTime("Deadline");
    const double max = askNumber("Maximum mark");
    const Id id = ctx_.assignments.createAssignment(user, course, title, text, deadline, max);
    out_ << "Assignment created with number " << id << ".\n";
}

void ConsoleApp::listAssignments(const User& user) {
    showCourses(user);
    const Id course = askId("Course number");
    const auto list = ctx_.assignments.listForCourse(user, course);
    if (list.empty()) out_ << "No assignments in this course.\n";
    for (const auto& a : list)
        out_ << "  [" << a.id << "] " << a.title << " | due " << a.deadline.toString() << " | max "
             << money(a.maxMark) << '\n';
}

void ConsoleApp::grantExtension(const User& user) {
    const Id assignment = askId("Assignment number");
    const Id student = askId("Student user number");
    const DateTime until = askDateTime("New deadline for this student");
    const std::string reason = ask("Reason for the extension");
    ctx_.assignments.grantExtension(user, assignment, student, until, reason);
    out_ << "Extension recorded.\n";
}

void ConsoleApp::viewSubmissions(const User& user) {
    const Id assignment = askId("Assignment number");
    const auto list = ctx_.assessment.submissionsForAssignment(user, assignment);
    if (list.empty()) out_ << "No submissions yet.\n";
    for (const auto& v : list) {
        out_ << "  [" << v.submission.id << "] " << v.studentUsername << " | "
             << toString(v.submission.status) << " | submitted " << v.submission.submittedAt.toString()
             << " | late days " << v.lateDays << " | file " << v.submission.filePath;
        if (v.submission.mark) out_ << " | mark " << money(*v.submission.mark) << "/" << money(v.maxMark);
        if (v.latestModeration && v.latestModeration->outcome == ModerationOutcome::CorrectionRequired &&
            v.submission.status == SubmissionStatus::Marked)
            out_ << "\n      HoD requests a correction: " << v.latestModeration->comments;
        out_ << '\n';
    }
}

void ConsoleApp::markSubmission(const User& user) {
    const Id id = askId("Submission number");
    const double raw = askNumber("Mark (before any late penalty)");
    const std::string feedback = ask("Written feedback");
    const std::string reason =
        ask("Reason (required when changing an existing mark; optional for a first mark)");
    const MarkResult r = ctx_.assessment.recordMark(user, id, raw, feedback, reason);
    out_ << "Mark saved. Entered " << money(r.rawMark);
    if (r.lateDays > 0) out_ << ", late " << r.lateDays << " day(s), penalty " << money(r.penalty);
    out_ << ", final mark " << money(r.finalMark)
         << ".\nIt will be visible to the student only after the Head of Department approves and releases "
            "it.\n";
}

void ConsoleApp::showAudit(const User& user) {
    const Id id = askId("Submission number");
    const auto trail = ctx_.assessment.auditTrail(user, id);
    if (trail.empty()) out_ << "No mark changes recorded.\n";
    for (const auto& e : trail) {
        out_ << "  " << e.audit.changedAt.toString() << " | " << e.changedByUsername << " | "
             << (e.audit.oldMark ? money(*e.audit.oldMark) : std::string("-")) << " -> "
             << money(e.audit.newMark) << " | " << e.audit.reason << '\n';
    }
}

// ---- head of department --------------------------------------------------------------------
void ConsoleApp::hodMenu(User& user) {
    const std::vector<std::string> items = {"Marks waiting for review or release",
                                            "Review a submission (approve or request correction)",
                                            "Release approved marks",
                                            "View mark history of a submission",
                                            "My department's courses",
                                            "Change my password"};
    for (;;) {
        const int c = askMenuChoice(items, "Head of Department menu");
        if (c == 0) return;
        guarded(out_, [&] {
            switch (c) {
                case 1:
                    showAwaiting(user);
                    break;
                case 2:
                    reviewSubmission(user);
                    break;
                case 3:
                    releaseMarks(user);
                    break;
                case 4:
                    showAudit(user);
                    break;
                case 5:
                    showCourses(user);
                    break;
                case 6:
                    changePassword(user);
                    break;
                default:
                    out_ << "Please choose a number from the menu.\n";
            }
        });
    }
}

void ConsoleApp::showAwaiting(const User& user) {
    const auto list = ctx_.assessment.awaitingModeration(user);
    if (list.empty()) out_ << "Nothing is waiting.\n";
    for (const auto& v : list) {
        out_ << "  [" << v.submission.id << "] " << v.courseName << " | " << v.assignmentTitle << " | "
             << v.studentUsername << " | mark " << money(*v.submission.mark) << "/" << money(v.maxMark)
             << " | "
             << (v.submission.status == SubmissionStatus::Marked ? "AWAITING REVIEW"
                                                                 : "APPROVED, awaiting release");
        if (v.lateDays > 0) out_ << " | " << v.lateDays << " day(s) late";
        out_ << '\n';
    }
}

void ConsoleApp::reviewSubmission(const User& user) {
    showAwaiting(user);
    const Id id = askId("Submission number to review");
    for (const auto& v : ctx_.assessment.awaitingModeration(user)) {
        if (v.submission.id != id) continue;
        out_ << "Student: " << v.studentUsername << "\nMark: " << money(*v.submission.mark) << " / "
             << money(v.maxMark) << "\nFeedback: " << v.submission.feedback << '\n';
        for (const auto& e : ctx_.assessment.auditTrail(user, id))
            out_ << "  history: " << e.audit.changedAt.toString() << " " << e.changedByUsername << " "
                 << (e.audit.oldMark ? money(*e.audit.oldMark) : std::string("-")) << " -> "
                 << money(e.audit.newMark) << " (" << e.audit.reason << ")\n";
    }
    const std::string decision = ask("Decision: (a)pprove, (c)orrection required, anything else cancels");
    if (decision == "a") {
        ctx_.assessment.moderate(user, id, ModerationOutcome::Approved, ask("Comments (optional)"));
        out_ << "Approved. Use 'Release approved marks' to make it visible to the student.\n";
    } else if (decision == "c") {
        ctx_.assessment.moderate(user, id, ModerationOutcome::CorrectionRequired,
                                 ask("Explain what must be corrected"));
        out_ << "Returned to the lecturer for correction.\n";
    } else {
        out_ << "Cancelled.\n";
    }
}

void ConsoleApp::releaseMarks(const User& user) {
    showCourses(user);
    const std::string which = ask(
        "Enter a submission number to release one mark, or 'c' to release all approved marks of a course");
    if (which == "c") {
        const int n = ctx_.assessment.releaseAllApproved(user, askId("Course number"));
        out_ << n << " mark(s) released to students.\n";
    } else {
        if (which.empty() || which.find_first_not_of("0123456789") != std::string::npos)
            throw ValidationError("Please enter a valid number.");
        ctx_.assessment.release(user, std::stoll(which));
        out_ << "Mark released to the student.\n";
    }
}

// ---- administrator -------------------------------------------------------------------------
void ConsoleApp::adminMenu(User& user) {
    const std::vector<std::string> items = {"Create a user account",
                                            "List users",
                                            "Activate or deactivate an account",
                                            "Reset a user's password",
                                            "Create a department",
                                            "Set a department's Head",
                                            "Create a course",
                                            "Assign a lecturer to a course",
                                            "Enrol a student in a course",
                                            "List departments and courses",
                                            "Change my password"};
    for (;;) {
        const int c = askMenuChoice(items, "Administrator menu");
        if (c == 0) return;
        guarded(out_, [&] {
            switch (c) {
                case 1:
                    createUser(user);
                    break;
                case 2:
                    listUsers(user);
                    break;
                case 3:
                    toggleActive(user);
                    break;
                case 4:
                    resetPassword(user);
                    break;
                case 5:
                    createDepartment(user);
                    break;
                case 6:
                    setHead(user);
                    break;
                case 7:
                    createCourse(user);
                    break;
                case 8:
                    assignLecturer(user);
                    break;
                case 9:
                    enrolStudent(user);
                    break;
                case 10:
                    listAcademic(user);
                    break;
                case 11:
                    changePassword(user);
                    break;
                default:
                    out_ << "Please choose a number from the menu.\n";
            }
        });
    }
}

void ConsoleApp::createUser(const User& user) {
    const std::string name = ask("New username");
    const std::string role = ask("Role (Student, Lecturer, HoD, Administrator)");
    const std::string pw = askSecret("Initial password (at least 8 characters)");
    const Id id = ctx_.userService.createUser(user, name, pw, roleFromString(role));
    out_ << "Account created with number " << id << ".\n";
}

void ConsoleApp::listUsers(const User& user) {
    for (const auto& u : ctx_.userService.listUsers(user))
        out_ << "  [" << u->id() << "] " << u->username() << " | " << u->roleName() << " | "
             << (u->isActive() ? "active" : "DEACTIVATED") << '\n';
}

void ConsoleApp::toggleActive(const User& user) {
    const Id id = askId("User number");
    const bool active = askYesNo("Should the account be ACTIVE");
    ctx_.userService.setActive(user, id, active);
    out_ << "Account " << (active ? "activated" : "deactivated") << ".\n";
}

void ConsoleApp::resetPassword(const User& user) {
    const Id id = askId("User number");
    ctx_.userService.resetPassword(user, id, askSecret("New password (at least 8 characters)"));
    out_ << "Password reset.\n";
}

void ConsoleApp::createDepartment(const User& user) {
    out_ << "Department number " << ctx_.academic.createDepartment(user, ask("Department name"))
         << " created.\n";
}

void ConsoleApp::setHead(const User& user) {
    listAcademic(user);
    const Id dept = askId("Department number");
    const Id hod = askId("User number of the Head of Department");
    ctx_.academic.setDepartmentHead(user, dept, hod);
    out_ << "Head of Department set.\n";
}

void ConsoleApp::createCourse(const User& user) {
    listAcademic(user);
    const Id dept = askId("Department number");
    out_ << "Course number " << ctx_.academic.createCourse(user, dept, ask("Course name")) << " created.\n";
}

void ConsoleApp::assignLecturer(const User& user) {
    const Id course = askId("Course number");
    const Id lecturer = askId("Lecturer user number");
    ctx_.academic.assignLecturer(user, course, lecturer);
    out_ << "Lecturer assigned.\n";
}

void ConsoleApp::enrolStudent(const User& user) {
    const Id student = askId("Student user number");
    const Id course = askId("Course number");
    ctx_.academic.enrolStudent(user, student, course);
    out_ << "Student enrolled.\n";
}

void ConsoleApp::listAcademic(const User& user) {
    out_ << "Departments:\n";
    for (const auto& d : ctx_.academic.listDepartments(user))
        out_ << "  [" << d.id << "] " << d.name
             << (d.headId ? " (head user " + std::to_string(*d.headId) + ")" : " (no head yet)") << '\n';
    out_ << "Courses:\n";
    for (const auto& c : ctx_.academic.listCourses(user))
        out_ << "  [" << c.id << "] " << c.name << " (department " << c.departmentId << ")\n";
}

}  // namespace lms
