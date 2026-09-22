#include <gtest/gtest.h>
#include "http/utils/JwtUtils.hpp"

namespace http_tests {

using namespace http::utils;

TEST(JwtUtilsTest, GenerateAndVerifyValidToken) {
    std::string original_user_id = "6aad29617c2ab2968702d3a3";
    std::string original_role = "admin";

    std::string token = JwtUtils::generateToken(original_user_id, original_role);
    EXPECT_FALSE(token.empty());

    std::string extracted_user_id;
    bool is_valid = JwtUtils::verifyToken(token, extracted_user_id);

    EXPECT_TRUE(is_valid);
    EXPECT_EQ(extracted_user_id, original_user_id);
}

TEST(JwtUtilsTest, RejectsEmptyOrGarbageToken) {
    std::string extracted_user_id;
    
    EXPECT_FALSE(JwtUtils::verifyToken("", extracted_user_id));
    EXPECT_FALSE(JwtUtils::verifyToken("not.a.real.jwt.token", extracted_user_id));
    EXPECT_FALSE(JwtUtils::verifyToken("eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXUyJ9.garbage.signature", extracted_user_id));
}

TEST(JwtUtilsTest, RejectsTamperedSignature) {
    std::string token = JwtUtils::generateToken("user_123", "user");
    
    size_t first_dot = token.find('.');
    
    if (first_dot != std::string::npos && first_dot + 1 < token.length()) {
        token[first_dot + 1] = (token[first_dot + 1] == 'a') ? 'b' : 'a';
    }
    
    std::string extracted_user_id;
    bool is_valid = JwtUtils::verifyToken(token, extracted_user_id);
    
    EXPECT_FALSE(is_valid);
}

} // namespace http_tests