#include "common/DateTime.h"

#include <cstdio>

#include "common/Errors.h"

namespace lms {
namespace {

// Howard Hinnant's public-domain civil-calendar algorithms.
std::int64_t daysFromCivil(int y, unsigned m, unsigned d) {
    y -= m <= 2;
    const std::int64_t era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + static_cast<std::int64_t>(doe) - 719468;
}

void civilFromDays(std::int64_t z, int& y, unsigned& m, unsigned& d) {
    z += 719468;
    const std::int64_t era = (z >= 0 ? z : z - 146096) / 146097;
    const unsigned doe = static_cast<unsigned>(z - era * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    y = static_cast<int>(yoe) + static_cast<int>(era) * 400;
    const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const unsigned mp = (5 * doy + 2) / 153;
    d = doy - (153 * mp + 2) / 5 + 1;
    m = mp < 10 ? mp + 3 : mp - 9;
    y += m <= 2;
}

bool isLeap(int y) {
    return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
}

int daysInMonth(int y, int m) {
    static const int dm[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    return (m == 2 && isLeap(y)) ? 29 : dm[m - 1];
}

const char* const kFormatHelp =
    "Invalid date or time. Use the format YYYY-MM-DD HH:MM (e.g. 2026-11-20 23:59).";

}  // namespace

DateTime DateTime::fromParts(int y, int mo, int d, int h, int mi, int s) {
    if (y < 2000 || y > 2100 || mo < 1 || mo > 12 || d < 1 || d > daysInMonth(y, mo) || h < 0 || h > 23 ||
        mi < 0 || mi > 59 || s < 0 || s > 59) {
        throw ValidationError(kFormatHelp);
    }
    const std::int64_t days = daysFromCivil(y, static_cast<unsigned>(mo), static_cast<unsigned>(d));
    return fromEpochSeconds(days * 86400 + h * 3600 + mi * 60 + s);
}

DateTime DateTime::parse(const std::string& text) {
    int y = 0, mo = 0, d = 0, h = 0, mi = 0, s = 0, consumed = 0;
    int n = std::sscanf(text.c_str(), "%d-%d-%d %d:%d:%d%n", &y, &mo, &d, &h, &mi, &s, &consumed);
    if (n == 6 && static_cast<std::size_t>(consumed) == text.size()) return fromParts(y, mo, d, h, mi, s);
    consumed = 0;
    n = std::sscanf(text.c_str(), "%d-%d-%d %d:%d%n", &y, &mo, &d, &h, &mi, &consumed);
    if (n == 5 && static_cast<std::size_t>(consumed) == text.size()) return fromParts(y, mo, d, h, mi, 0);
    throw ValidationError(kFormatHelp);
}

std::string DateTime::toString() const {
    std::int64_t days = epoch_ / 86400;
    std::int64_t rem = epoch_ % 86400;
    if (rem < 0) {
        rem += 86400;
        --days;
    }
    int y = 0;
    unsigned m = 0, d = 0;
    civilFromDays(days, y, m, d);
    char buf[32];
    std::snprintf(buf, sizeof buf, "%04d-%02u-%02u %02d:%02d:%02d", y, m, d, static_cast<int>(rem / 3600),
                  static_cast<int>((rem % 3600) / 60), static_cast<int>(rem % 60));
    return buf;
}

}  // namespace lms
