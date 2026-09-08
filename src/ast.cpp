#include "ast.h"
#include "parser.h"
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <fstream>
#include <functional>
#include <cctype>
#include <cmath>
#include <random>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <algorithm>
#include <limits>

std::string NullLiteral::toString() const {
    return "null";
}

std::string NumberLiteral::toString() const {
    return std::to_string(value);
}

std::string FloatLiteral::toString() const {
    return std::to_string(value);
}

std::string BreakStatement::toString() const {
    return "break";
}

std::string ContinueStatement::toString() const {
    return "continue";
}

std::string BooleanLiteral::toString() const {
    return value ? "True" : "False";
}

std::string StringLiteral::toString() const {
    return "\"" + value + "\"";
}

std::string Identifier::toString() const {
    return name;
}

std::string UnaryOp::toString() const {
    return "(" + op + " " + operand->toString() + ")";
}

std::string BinaryOp::toString() const {
    return "(" + left->toString() + " " + op + " " + right->toString() + ")";
}

std::string CallExpression::toString() const {
    std::string s = callee + "(";

    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) s += ", ";
        s += args[i]->toString();
    }

    s += ")";
    return s;
}

std::string AttributeAccess::toString() const {
    return object->toString() + "." + attribute;
}

std::string SliceAccess::toString() const {
    return object->toString() + "[" + (start ? start->toString() : "") + ":" + (end ? end->toString() : "") + "]";
}

std::string IndexAccess::toString() const {
    return object->toString() + "[" + index->toString() + "]";
}

std::string IndexAssignment::toString() const {
    return object->toString() + "[" + index->toString() + "] " + op + " " + expr->toString();
}

std::string Assignment::toString() const {
    return var + " = " + expr->toString();
}

std::string ListLiteral::toString() const {
    std::string s = "[";

    for (size_t i = 0; i < elements.size(); ++i) {
        if (i > 0) s += ", ";
        s += elements[i]->toString();
    }

    return s + "]";
}

std::string DictLiteral::toString() const {
    std::string s = "{";

    for (size_t i = 0; i < entries.size(); ++i) {
        if (i > 0) s += ", ";

        s += entries[i].first->toString();
        s += ": ";
        s += entries[i].second->toString();
    }

    return s + "}";
}

std::string PrintStatement::toString() const {
    return "print(" + expr->toString() + ")";
}

std::string ReturnStatement::toString() const {
    return expr ? "return " + expr->toString() : "return";
}

std::string FunctionDef::toString() const {
    std::string s = "def " + name + "(";

    for (size_t i = 0; i < parameters.size(); ++i) {
        if (i > 0) s += ", ";
        s += parameters[i];
    }

    if (!variadicParameter.empty()) {
        if (!parameters.empty()) s += ", ";
        s += "*" + variadicParameter;
    }

    s += "):\n";

    for (const auto& stmt : body) {
        s += "  " + stmt->toString() + "\n";
    }

    return s;
}

std::string ClassDef::toString() const {
    std::string s = "class " + name;

    if (!baseClass.empty()) {
        s += " extends " + baseClass;
    }

    s += ":\n";

    for (const auto& stmt : body) {
        s += "  " + stmt->toString() + "\n";
    }

    return s;
}

std::string ForStatement::toString() const {
    std::string s = "for " + var + " in " + iterable->toString() + ":\n";

    for (const auto& stmt : body) {
        s += "  " + stmt->toString() + "\n";
    }

    return s;
}

std::string ThrowStatement::toString() const {
    return "throw " + (expr ? expr->toString() : "");
}

std::string TryStatement::toString() const {
    std::string s = "try:\n";

    for (const auto& stmt : tryBody) {
        s += "  " + stmt->toString() + "\n";
    }

    s += "catch " + exceptionName + ":\n";

    for (const auto& stmt : catchBody) {
        s += "  " + stmt->toString() + "\n";
    }

    return s;
}

std::string IfStatement::toString() const {
    std::string s = "if " + condition->toString() + ":\n";

    for (const auto& stmt : body) {
        s += "  " + stmt->toString() + "\n";
    }

    if (!elseBody.empty()) {
        s += "else:\n";

        for (const auto& stmt : elseBody) {
            s += "  " + stmt->toString() + "\n";
        }
    }

    return s;
}

std::string WhileStatement::toString() const {
    std::string s = "while " + condition->toString() + ":\n";

    for (const auto& stmt : body) {
        s += "  " + stmt->toString() + "\n";
    }

    return s;
}

std::string Program::toString() const {
    std::string result = "Program:\n";

    for (const auto& stmt : statements) {
        result += "  " + stmt->toString() + "\n";
    }

    return result;
}
