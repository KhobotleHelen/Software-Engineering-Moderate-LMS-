// Portable UTC date-time value type ("YYYY-MM-DD HH:MM:SS"). Own calendar maths so
// that behaviour is identical on Windows, Linux and macOS (no timegm/gmtime_r).
#pragma once
#include <cstdint>
#include <string>

namespace lms {

class DateTime {
public:
    DateTime() = default;
    static DateTime fromEpochSeconds(std::int64_t s) {
        DateTime d;
        d.epoch_ = s;
        return d;
    }
    // Accepts "YYYY-MM-DD HH:MM" or "YYYY-MM-DD HH:MM:SS". Throws ValidationError.
    static DateTime parse(const std::string& text);
    static DateTime fromParts(int y, int mo, int d, int h = 0, int mi = 0, int s = 0);

    std::int64_t epochSeconds() const { return epoch_; }
    std::string toString() const;  // always "YYYY-MM-DD HH:MM:SS"
    DateTime plusSeconds(std::int64_t s) const { return fromEpochSeconds(epoch_ + s); }
    DateTime plusDays(int d) const { return plusSeconds(static_cast<std::int64_t>(d) * 86400); }

    friend bool operator==(const DateTime& a, const DateTime& b) { return a.epoch_ == b.epoch_; }
    friend bool operator!=(const DateTime& a, const DateTime& b) { return a.epoch_ != b.epoch_; }
    friend bool operator<(const DateTime& a, const DateTime& b) { return a.epoch_ < b.epoch_; }
    friend bool operator<=(const DateTime& a, const DateTime& b) { return a.epoch_ <= b.epoch_; }
    friend bool operator>(const DateTime& a, const DateTime& b) { return a.epoch_ > b.epoch_; }
    friend bool operator>=(const DateTime& a, const DateTime& b) { return a.epoch_ >= b.epoch_; }

private:
    std::int64_t epoch_ = 0;
};

}  // namespace lms
