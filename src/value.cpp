#include "value.h"
#include <sstream>
namespace tekst {
std::string valueToString(const Value& value){switch(value.kind){case ValueKind::None:return "None";case ValueKind::Integer:return std::to_string(value.integer);case ValueKind::Float:{std::ostringstream s;s<<value.floating;return s.str();}case ValueKind::Boolean:return value.boolean?"true":"false";case ValueKind::String:return value.string;}return "None";}
}
