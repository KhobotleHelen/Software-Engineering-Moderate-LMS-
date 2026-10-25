// Equivalence partitioning + boundary value analysis of the late-penalty rule (Phase 5 test strategy).
#include <gtest/gtest.h>

#include "strategy/LatePenalty.h"

using namespace lms;

namespace {
const DateTime kDeadline = DateTime::parse("2026-11-20 23:59:00");
constexpr std::int64_t kDay = 86400;
}  // namespace

struct LateDaysCase {
    std::int64_t offsetSeconds;
    int expectedDays;
};

class LateDaysBoundary : public ::testing::TestWithParam<LateDaysCase> {};

TEST_P(LateDaysBoundary, CountsWholeDaysAndRoundsUp) {
    StandardLatePenalty s;
    const auto c = GetParam();
    EXPECT_EQ(s.lateDays(kDeadline, kDeadline.plusSeconds(c.offsetSeconds)), c.expectedDays);
}

INSTANTIATE_TEST_SUITE_P(Boundaries, LateDaysBoundary,
                         ::testing::Values(LateDaysCase{-kDay, 0}, LateDaysCase{-1, 0},
                                           LateDaysCase{0, 0},                                    // on time
                                           LateDaysCase{1, 1}, LateDaysCase{kDay, 1},             // day 1
                                           LateDaysCase{kDay + 1, 2}, LateDaysCase{2 * kDay, 2},  // day 2
                                           LateDaysCase{2 * kDay + 1, 3}, LateDaysCase{3 * kDay, 3},  // day 3
                                           LateDaysCase{3 * kDay + 1, 4},
                                           LateDaysCase{30 * kDay, 30}));  // over limit

struct MarkCase {
    int days;
    double raw;
    double expected;
};
class PenaltyMarks : public ::testing::TestWithParam<MarkCase> {};

TEST_P(PenaltyMarks, AppliesTenPercentOfMaxPerDayThenZero) {
    StandardLatePenalty s;
    const auto c = GetParam();
    EXPECT_DOUBLE_EQ(s.apply(c.raw, 100.0, c.days), c.expected);
}

INSTANTIATE_TEST_SUITE_P(Partitions, PenaltyMarks,
                         ::testing::Values(MarkCase{0, 80, 80}, MarkCase{1, 80, 70}, MarkCase{2, 80, 60},
                                           MarkCase{3, 80, 50}, MarkCase{4, 80, 0}, MarkCase{9, 100, 0},
                                           MarkCase{3, 20,
                                                    0},  // penalty larger than the mark never goes negative
                                           MarkCase{1, 0, 0}, MarkCase{1, 100, 90}));

TEST(LatePenalty, PenaltyIsTenPercentOfMaximumNotOfTheMarkEarned) {
    StandardLatePenalty s;
    EXPECT_DOUBLE_EQ(s.penaltyAmount(100.0, 1), 10.0);  // the worked example in the Phase 3 SDD
    EXPECT_DOUBLE_EQ(s.penaltyAmount(50.0, 2), 10.0);
    EXPECT_DOUBLE_EQ(s.penaltyAmount(100.0, 0), 0.0);
    EXPECT_DOUBLE_EQ(s.penaltyAmount(100.0, 4), 100.0);
    EXPECT_FALSE(s.exceedsLimit(3));
    EXPECT_TRUE(s.exceedsLimit(4));
}

TEST(LatePenalty, StrategyIsReplaceableViaInterface) {  // OCP: a different school policy, no service change
    const StandardLatePenalty strict(0.5, 1);
    const ILatePenaltyStrategy& i = strict;
    EXPECT_DOUBLE_EQ(i.apply(80, 100, 1), 30.0);
    EXPECT_DOUBLE_EQ(i.apply(80, 100, 2), 0.0);
}
