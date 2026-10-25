#include <gtest/gtest.h>

#include <cmath>

#include "common/DateTime.h"
#include "common/Errors.h"
#include "common/Validation.h"

using namespace lms;

TEST(DateTime, ParseAndFormatRoundTrip) {
    const DateTime d = DateTime::parse("2026-11-20 23:59");
    EXPECT_EQ(d.toString(), "2026-11-20 23:59:00");
    EXPECT_EQ(DateTime::parse(d.toString()), d);
    EXPECT_EQ(DateTime::parse("2000-01-01 00:00:00").epochSeconds(), 946684800);
}

TEST(DateTime, LeapYearHandling) {
    EXPECT_NO_THROW(DateTime::parse("2028-02-29 10:00"));
    EXPECT_THROW(DateTime::parse("2027-02-29 10:00"), ValidationError);
    EXPECT_EQ(DateTime::parse("2028-02-28 12:00").plusDays(1).toString(), "2028-02-29 12:00:00");
}

TEST(DateTime, RejectsInvalidInput) {
    for (const char* bad : {"", "tomorrow", "2026-13-01 10:00", "2026-11-31 10:00", "2026-11-20 24:00",
                            "2026-11-20 10:60", "2026-11-20", "2026-11-20 10:00 extra", "1999-01-01 00:00"})
        EXPECT_THROW(DateTime::parse(bad), ValidationError) << bad;
}

TEST(DateTime, ArithmeticAndOrdering) {
    const DateTime a = DateTime::parse("2026-12-31 23:59:59");
    EXPECT_EQ(a.plusSeconds(1).toString(), "2027-01-01 00:00:00");
    EXPECT_TRUE(a < a.plusSeconds(1));
    EXPECT_TRUE(a <= a);
    EXPECT_TRUE(a.plusDays(1) > a);
    EXPECT_NE(a, a.plusSeconds(1));
}

TEST(Validation, Username) {
    EXPECT_NO_THROW(validation::requireUsername("abc"));
    EXPECT_NO_THROW(validation::requireUsername("a.b_c-9"));
    EXPECT_THROW(validation::requireUsername("ab"), ValidationError);                  // too short
    EXPECT_THROW(validation::requireUsername(std::string(33, 'a')), ValidationError);  // too long
    EXPECT_THROW(validation::requireUsername("bad name"), ValidationError);
    EXPECT_THROW(validation::requireUsername("x'; DROP TABLE User;--"), ValidationError);
}

TEST(Validation, PasswordBoundaries) {
    EXPECT_THROW(validation::requirePassword(std::string(7, 'a')), ValidationError);
    EXPECT_NO_THROW(validation::requirePassword(std::string(8, 'a')));
    EXPECT_NO_THROW(validation::requirePassword(std::string(128, 'a')));
    EXPECT_THROW(validation::requirePassword(std::string(129, 'a')), ValidationError);
}

TEST(Validation, MarkBoundaries) {
    EXPECT_NO_THROW(validation::requireMark(0, 100));
    EXPECT_NO_THROW(validation::requireMark(100, 100));
    EXPECT_THROW(validation::requireMark(-0.01, 100), ValidationError);
    EXPECT_THROW(validation::requireMark(100.01, 100), ValidationError);
    EXPECT_THROW(validation::requireMark(std::nan(""), 100), ValidationError);
    EXPECT_THROW(validation::requireMark(INFINITY, 100), ValidationError);
}

TEST(Validation, MaxMarkAndText) {
    EXPECT_THROW(validation::requirePositiveMaxMark(0), ValidationError);
    EXPECT_THROW(validation::requirePositiveMaxMark(1000.5), ValidationError);
    EXPECT_NO_THROW(validation::requirePositiveMaxMark(0.5));
    EXPECT_THROW(validation::requireNonEmpty("   ", "Title"), ValidationError);
    EXPECT_THROW(validation::requireNonEmpty(std::string(11, 'x'), "Title", 10), ValidationError);
    EXPECT_EQ(validation::trim("  hi \t"), "hi");
}
