#include "shadowsyscall.hpp"
#include "gtest/gtest.h"

constexpr auto test_address = 0x7ffe0000;

static_assert( std::is_same_v<decltype( std::declval<shadow::address_t>().ptr<uint32_t>() ), uint32_t*> );

TEST( address, test_suite ) {
    uint32_t* memory_ptr = reinterpret_cast<uint32_t*>( test_address );
    shadow::address_t address = memory_ptr;

    EXPECT_EQ( address, test_address );
    EXPECT_EQ( address, memory_ptr );

    EXPECT_EQ( address.offset( 4 ), test_address + 4 );
    EXPECT_EQ( address.ptr<uint32_t>( 4 ), memory_ptr + 4 );
}