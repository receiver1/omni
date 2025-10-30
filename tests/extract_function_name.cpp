#include "gtest/gtest.h"
#include "shadowsyscall.hpp"

using shadow::detail::extract_function_name;

int MessageBoxA(void* hWnd, const char* lpText, const char* lpCaption, uint32_t uType);
int MessageBoxW(void* hWnd, const char* lpText, const char* lpCaption, uint32_t uType);
#define MessageBox MessageBoxA

TEST(extract_function_name, basics) {
  EXPECT_EQ(extract_function_name<MessageBoxA>(), "MessageBoxA");
  EXPECT_EQ(extract_function_name<MessageBoxW>(), "MessageBoxW");
  EXPECT_EQ(extract_function_name<MessageBox>(), "MessageBoxA");

  EXPECT_EQ(extract_function_name<std::memcpy>(), "memcpy");
}