#include "common/Validation.h"

#include <algorithm>
#include <cctype>
#include <cmath>

#include "common/Errors.h"

namespace lms::validation {

std::string trim(const std::string& s) {
    std::size_t b = 0, e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return s.substr(b, e - b);
}

void requireNonEmpty(const std::string& value, const std::string& fieldName, std::size_t maxLen) {
    if (trim(value).empty()) throw ValidationError(fieldName + " must not be empty.");
    if (value.size() > maxLen)
        throw ValidationError(fieldName + " is too long (maximum " + std::to_string(maxLen) +
                              " characters).");
}

void requireUsername(const std::string& username) {
    if (username.size() < 3 || username.size() > 32)
        throw ValidationError("Username must be 3 to 32 characters long.");
    const bool allowed = std::all_of(username.begin(), username.end(), [](unsigned char c) {
        return std::isalnum(c) || c == '.' || c == '_' || c == '-';
    });
    if (!allowed) throw ValidationError("Username may only contain letters, digits, '.', '_' and '-'.");
}

void requirePassword(const std::string& password) {
    if (password.size() < 8) throw ValidationError("Password must be at least 8 characters long.");
    if (password.size() > 128) throw ValidationError("Password must be at most 128 characters long.");
}

void requireMark(double mark, double maxMark) {
    if (!std::isfinite(mark)) throw ValidationError("Mark must be a number.");
    if (mark < 0.0) throw ValidationError("Mark cannot be negative.");
    if (mark > maxMark)
        throw ValidationError("Mark cannot be greater than the assignment maximum (" +
                              std::to_string(maxMark) + ").");
}

void requirePositiveMaxMark(double maxMark) {
    if (!std::isfinite(maxMark) || maxMark <= 0.0 || maxMark > 1000.0)
        throw ValidationError("Maximum mark must be greater than 0 and at most 1000.");
}

}  // namespace lms::validation
