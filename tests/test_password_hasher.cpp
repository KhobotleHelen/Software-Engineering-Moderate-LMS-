#include <gtest/gtest.h>

#include "security/PasswordHasher.h"

using namespace lms;

TEST(PasswordHasher, VerifiesCorrectAndRejectsWrongPassword) {
    SodiumPasswordHasher h(SodiumPasswordHasher::Profile::Fast);
    const auto p = h.hash("correct horse");
    EXPECT_TRUE(h.verify("correct horse", p.hash, p.salt));
    EXPECT_FALSE(h.verify("Correct horse", p.hash, p.salt));
    EXPECT_FALSE(h.verify("", p.hash, p.salt));
}

TEST(PasswordHasher, SamePasswordGetsDifferentSaltAndHash) {
    SodiumPasswordHasher h(SodiumPasswordHasher::Profile::Fast);
    const auto a = h.hash("same-password");
    const auto b = h.hash("same-password");
    EXPECT_NE(a.salt, b.salt);
    EXPECT_NE(a.hash, b.hash);
}

TEST(PasswordHasher, StoredFormIsHexAndNeverContainsThePassword) {
    SodiumPasswordHasher h(SodiumPasswordHasher::Profile::Fast);
    const auto p = h.hash("plaintext-secret");
    EXPECT_EQ(p.hash.size(), 64u);
    EXPECT_EQ(p.salt.size(), 32u);
    EXPECT_EQ(p.hash.find("plaintext-secret"), std::string::npos);
    EXPECT_EQ(p.hash.find_first_not_of("0123456789abcdef"), std::string::npos);
}

TEST(PasswordHasher, MalformedStoredValuesFailSafely) {
    SodiumPasswordHasher h(SodiumPasswordHasher::Profile::Fast);
    const auto p = h.hash("pw");
    EXPECT_FALSE(h.verify("pw", "nothex", p.salt));
    EXPECT_FALSE(h.verify("pw", p.hash, "zz"));
    EXPECT_FALSE(h.verify("pw", "", ""));
    EXPECT_FALSE(h.verify("pw", p.hash.substr(0, 10), p.salt));
}

TEST(PasswordHasher, ProductionProfileInterOperatesWithItself) {
    SodiumPasswordHasher h;  // Interactive (production) parameters
    const auto p = h.hash("production-pw");
    EXPECT_TRUE(h.verify("production-pw", p.hash, p.salt));
    SodiumPasswordHasher fast(SodiumPasswordHasher::Profile::Fast);
    EXPECT_FALSE(
        fast.verify("production-pw", p.hash, p.salt));  // different cost parameters => different hash
}
