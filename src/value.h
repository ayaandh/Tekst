#pragma once
#include <cstdint>
#include <string>
namespace tekst {
enum class ValueKind { None, Integer, Float, Boolean, String };
struct Value { ValueKind kind=ValueKind::None; std::int64_t integer=0; double floating=0.0; bool boolean=false; std::string string; };
std::string valueToString(const Value& value);
}
