#include "shadowsyscall.hpp"
#include "gtest/gtest.h"

TEST( syscaller, syscall_result ) {
    constexpr auto invalid_handle = 0xFFFF;
    constexpr auto status_invalid_handle = 0xC0000008;

    using NTSTATUS = long;
    auto result = shadowsyscall<NTSTATUS>( "NtClose", invalid_handle );

    EXPECT_EQ( result.value, status_invalid_handle );
    ASSERT_TRUE( result.error.has_value() == false || result.error.value() == shadow::errc::none );
}
