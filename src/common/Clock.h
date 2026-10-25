// Clock abstraction: lets tests control "now" (deadline boundary testing).
#pragma once
#include <chrono>

#include "common/DateTime.h"

namespace lms {

class IClock {
public:
    virtual ~IClock() = default;
    virtual DateTime now() const = 0;
};

class SystemClock : public IClock {
public:
    DateTime now() const override {
        using namespace std::chrono;
        return DateTime::fromEpochSeconds(
            duration_cast<seconds>(system_clock::now().time_since_epoch()).count());
    }
};

class FixedClock : public IClock {
public:
    explicit FixedClock(DateTime t) : t_(t) {}
    DateTime now() const override { return t_; }
    void set(DateTime t) { t_ = t; }

private:
    DateTime t_;
};

}  // namespace lms
