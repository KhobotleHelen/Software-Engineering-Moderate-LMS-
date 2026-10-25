// Strategy pattern: late-penalty policy (Phase 3, OCP). The school can change the rule by
// supplying a different ILatePenaltyStrategy without touching SubmissionService/AssessmentService.
#pragma once
#include "common/DateTime.h"

namespace lms {

class ILatePenaltyStrategy {
public:
    virtual ~ILatePenaltyStrategy() = default;
    // Whole days late (0 = on time). Part of a day counts as a full day.
    virtual int lateDays(const DateTime& effectiveDeadline, const DateTime& submittedAt) const = 0;
    // True when the work is beyond the last day that earns marks.
    virtual bool exceedsLimit(int lateDays) const = 0;
    // Amount deducted from the mark (never more than maxMark).
    virtual double penaltyAmount(double maxMark, int lateDays) const = 0;
    // Mark after penalty, clamped to [0, rawMark].
    virtual double apply(double rawMark, double maxMark, int lateDays) const = 0;
};

// HoD rule: 10% of the available mark per day late for up to 3 days; after that, zero.
class StandardLatePenalty final : public ILatePenaltyStrategy {
public:
    explicit StandardLatePenalty(double ratePerDay = 0.10, int maxLateDays = 3)
        : rate_(ratePerDay), maxDays_(maxLateDays) {}
    int lateDays(const DateTime& effectiveDeadline, const DateTime& submittedAt) const override;
    bool exceedsLimit(int lateDays) const override { return lateDays > maxDays_; }
    double penaltyAmount(double maxMark, int lateDays) const override;
    double apply(double rawMark, double maxMark, int lateDays) const override;

private:
    double rate_;
    int maxDays_;
};

}  // namespace lms
