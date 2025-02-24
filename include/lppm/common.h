#pragma once
#include <cstddef>
#include <cstdint>

#define UNUSED(x) (void)(x)

#define STYLE_GREEN "\033[32m"
#define STYLE_CYAN "\033[36m"
#define STYLE_RED "\033[31m"
#define STYLE_BLUE "\033[34m"
#define STYLE_YELLOW "\033[33m"
#define STYLE_ITALIC "\033[3m"
#define STYLE_COLOR_RESET "\033[39m"
#define STYLE_RESET "\033[0m"

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using usz = std::size_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using c8 = char;
using c16 = wchar_t;
