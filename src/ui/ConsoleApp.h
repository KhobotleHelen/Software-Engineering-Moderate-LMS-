// Presentation layer: role-specific text menus. Contains no business rules and no SQL; every action
// delegates to a service. Streams are injected so the whole UI can be driven by scripted input in tests.
#pragma once
#include <iosfwd>

#include "services/AppContext.h"

namespace lms {

class InputClosed : public std::exception {};  // thrown when the input stream ends

class ConsoleApp {
public:
    ConsoleApp(AppContext& ctx, std::istream& in, std::ostream& out, const IClock& clock);
    void run();  // setup (if needed) -> login loop -> role menu

private:
    // input helpers
    std::string ask(const std::string& prompt);
    std::string askSecret(const std::string& prompt);
    Id askId(const std::string& prompt);
    double askNumber(const std::string& prompt);
    DateTime askDateTime(const std::string& prompt);
    bool askYesNo(const std::string& prompt);
    int askMenuChoice(const std::vector<std::string>& items, const std::string& title);

    // flows
    void firstRunSetup();
    std::unique_ptr<User> loginScreen();
    void roleMenu(User& user);
    void studentMenu(User& user);
    void lecturerMenu(User& user);
    void hodMenu(User& user);
    void adminMenu(User& user);
    void changePassword(User& user);

    // student
    void showCourses(const User& user);
    void showStudentAssignments(const User& user);
    void submitWork(const User& user);
    void showStudentResults(const User& user);
    // lecturer
    void createAssignment(const User& user);
    void listAssignments(const User& user);
    void grantExtension(const User& user);
    void viewSubmissions(const User& user);
    void markSubmission(const User& user);
    void showAudit(const User& user);
    // hod
    void showAwaiting(const User& user);
    void reviewSubmission(const User& user);
    void releaseMarks(const User& user);
    // admin
    void createUser(const User& user);
    void listUsers(const User& user);
    void toggleActive(const User& user);
    void resetPassword(const User& user);
    void createDepartment(const User& user);
    void setHead(const User& user);
    void createCourse(const User& user);
    void assignLecturer(const User& user);
    void enrolStudent(const User& user);
    void listAcademic(const User& user);

    static std::string money(double v);

    AppContext& ctx_;
    std::istream& in_;
    std::ostream& out_;
    const IClock& clock_;
};

}  // namespace lms
