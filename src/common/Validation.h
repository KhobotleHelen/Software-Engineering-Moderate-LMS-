// Input validation helpers. All throw ValidationError with a user-friendly message.
#pragma once
#include <string>

namespace lms::validation {

std::string trim(const std::string& s);
void requireNonEmpty(const std::string& value, const std::string& fieldName, std::size_t maxLen = 2000);
void requireUsername(const std::string& username);  // 3-32 chars: letters, digits, . _ -
void requirePassword(const std::string& password);  // 8-128 chars
void requireMark(double mark, double maxMark);      // 0 <= mark <= maxMark, finite
void requirePositiveMaxMark(double maxMark);        // 0 < max <= 1000, finite

}  // namespace lms::validation
