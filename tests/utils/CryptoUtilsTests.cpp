#include <gtest/gtest.h>
#include "http/utils/CryptoUtils.hpp"

namespace http_tests {

using namespace http::utils;

const char* PREFIX_IDENTIFIER_FOR_OUTPUT = "$argon2id$";

TEST(CryptoUtilsTest, GeneratesValidArgon2idHash) {
    std::string hash = CryptoUtils::hashPassword("SecurePass123!");
    
    EXPECT_FALSE(hash.empty());
    EXPECT_EQ(hash.substr(0, 10), PREFIX_IDENTIFIER_FOR_OUTPUT);
}

TEST(CryptoUtilsTest, VerifiesCorrectPassword) {
    std::string hash = CryptoUtils::hashPassword("CorrectHorseBatteryStaple");
    
    EXPECT_TRUE(CryptoUtils::verifyPassword("CorrectHorseBatteryStaple", hash));
}

TEST(CryptoUtilsTest, RejectsIncorrectPassword) {
    std::string hash = CryptoUtils::hashPassword("MySecretPassword");
    
    EXPECT_FALSE(CryptoUtils::verifyPassword("WrongPassword", hash));
    EXPECT_FALSE(CryptoUtils::verifyPassword("mysecretpassword", hash)); // Case-sensitive
    EXPECT_FALSE(CryptoUtils::verifyPassword("", hash));
}

TEST(CryptoUtilsTest, HashesAreUniqueDueToSalting) {
    // Cryptographic salting means the same password hashed twice MUST yield completely different strings
    std::string hash1 = CryptoUtils::hashPassword("SamePassword");
    std::string hash2 = CryptoUtils::hashPassword("SamePassword");
    
    EXPECT_NE(hash1, hash2);
    
    // Both unique hashes should still verify the same plaintext password
    EXPECT_TRUE(CryptoUtils::verifyPassword("SamePassword", hash1));
    EXPECT_TRUE(CryptoUtils::verifyPassword("SamePassword", hash2));
}

} // namespace http_tests