#include "strategy/LatePenalty.h"

#include <algorithm>

namespace lms {

int StandardLatePenalty::lateDays(const DateTime& effectiveDeadline, const DateTime& submittedAt) const {
    const std::int64_t diff = submittedAt.epochSeconds() - effectiveDeadline.epochSeconds();
    if (diff <= 0) return 0;                          // exactly at the deadline is still on time
    return static_cast<int>((diff + 86399) / 86400);  // ceil: 1 second late = 1 day late
}

double StandardLatePenalty::penaltyAmount(double maxMark, int lateDays) const {
    if (lateDays <= 0) return 0.0;
    if (exceedsLimit(lateDays)) return maxMark;
    return std::min(maxMark, maxMark * rate_ * lateDays);
}

double StandardLatePenalty::apply(double rawMark, double maxMark, int lateDays) const {
    if (lateDays <= 0) return rawMark;
    if (exceedsLimit(lateDays)) return 0.0;
    return std::max(0.0, rawMark - penaltyAmount(maxMark, lateDays));
}

}  // namespace lms
