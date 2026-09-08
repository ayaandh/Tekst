#include "value.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>
std::string valueToString(const RuntimeValue& value) {
    if (const auto* v = std::get_if<int>(&value)) {
        return std::to_string(*v);
    }

    if (const auto* v = std::get_if<bool>(&value)) {
        return *v ? "True" : "False";
    }

    if (const auto* v = std::get_if<std::string>(&value)) {
        return *v;
    }

    if (const auto* v = std::get_if<double>(&value)) {
        std::string s = std::to_string(*v);
        while (!s.empty() && s.back() == '0') s.pop_back();
        if (!s.empty() && s.back() == '.') s.pop_back();
        return s;
    }

    if (const auto* v = std::get_if<std::shared_ptr<RuntimeFunction>>(&value)) {
        return "<function " + (*v)->name + ">";
    }

    if (const auto* v = std::get_if<std::shared_ptr<RuntimeObject>>(&value)) {
        if ((*v)->className == "__list__") {
            std::string s = "[";
            bool first = true;

            for (size_t i = 0; i < 1000; ++i) {
                auto it = (*v)->fields.find(std::to_string(i));

                if (it == (*v)->fields.end()) {
                    break;
                }

                if (!first) {
                    s += ", ";
                }

                s += valueToString(it->second);
                first = false;
            }

            return s + "]";
        }

        if ((*v)->className == "__dict__") {
            std::string s = "{";
            bool first = true;

            for (const auto& [k, val] : (*v)->fields) {
                if (k == "__size__" || k == "__type__") {
                    continue;
                }

                if (!first) {
                    s += ", ";
                }

                s += k + ": " + valueToString(val);
                first = false;
            }

            return s + "}";
        }

        return "<" + (*v)->className + " object>";
    }

    return "<unknown>";
}

int asInt(const RuntimeValue& value) {
    if (const auto* v = std::get_if<int>(&value)) {
        return *v;
    }

    if (const auto* v = std::get_if<bool>(&value)) {
        return *v ? 1 : 0;
    }

    throw std::runtime_error("Expected numeric value");
}

bool asBool(const RuntimeValue& value) {
    if (const auto* v = std::get_if<int>(&value)) {
        return *v != 0;
    }

    if (const auto* v = std::get_if<bool>(&value)) {
        return *v;
    }

    if (const auto* v = std::get_if<std::string>(&value)) {
        return !v->empty();
    }

    return true;
}


int levenshteinDistance(const std::string& a, const std::string& b) {
    std::vector<int> prev(b.size()+1), cur(b.size()+1);
    for (size_t j=0;j<=b.size();++j) prev[j]=static_cast<int>(j);
    for (size_t i=1;i<=a.size();++i) {
        cur[0]=static_cast<int>(i);
        for (size_t j=1;j<=b.size();++j) {
            int cost=(std::tolower(static_cast<unsigned char>(a[i-1]))==std::tolower(static_cast<unsigned char>(b[j-1])))?0:1;
            cur[j]=std::min({prev[j]+1,cur[j-1]+1,prev[j-1]+cost});
        }
        prev.swap(cur);
    }
    return prev[b.size()];
}

std::string suggestName(const std::string& name, const std::map<std::string, RuntimeValue>& scope, const std::map<std::string, RuntimeValue>& globals) {
    std::string best;
    int bestDistance=3;
    auto consider=[&](const std::string& candidate){
        if (candidate==name) return;
        int d=levenshteinDistance(name,candidate);
        if (d<bestDistance) { bestDistance=d; best=candidate; }
    };
    for (const auto& [k,v]:scope) consider(k);
    for (const auto& [k,v]:globals) consider(k);
    return best;
}

std::string withSuggestion(const std::string& message, const std::string& name, const std::map<std::string, RuntimeValue>& scope, const std::map<std::string, RuntimeValue>& globals) {
    std::string suggestion=suggestName(name,scope,globals);
    if (suggestion.empty()) return message;
    return message + "\nDid you mean '" + suggestion + "'?";
}

std::string parserError(const Token& token, const std::string& message) {
    return "error: " + message + " at line " + std::to_string(token.line) + ", column " + std::to_string(token.column);
}

