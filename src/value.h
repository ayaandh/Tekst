#pragma once
#include "parser.h"
std::string valueToString(const RuntimeValue& value);
int asInt(const RuntimeValue& value);
bool asBool(const RuntimeValue& value);
int levenshteinDistance(const std::string& a, const std::string& b);
std::string suggestName(const std::string& name, const std::map<std::string, RuntimeValue>& scope, const std::map<std::string, RuntimeValue>& globals);
std::string withSuggestion(const std::string& message, const std::string& name, const std::map<std::string, RuntimeValue>& scope, const std::map<std::string, RuntimeValue>& globals);
std::string parserError(const Token& token, const std::string& message);
