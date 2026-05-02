#include "mcp/json.hpp"

#include <cmath>
#include <cstdio>
#include <limits>

namespace mcp {
namespace {

const int kMaxDepth = 64;

bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

int hex_value(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

void append_utf8(std::string& out, unsigned long codepoint)
{
    if (codepoint > 0x10FFFFUL || (codepoint >= 0xD800UL && codepoint <= 0xDFFFUL)) {
        codepoint = 0xFFFDUL;
    }
    if (codepoint <= 0x7FUL) {
        out.push_back(static_cast<char>(codepoint));
    } else if (codepoint <= 0x7FFUL) {
        out.push_back(static_cast<char>(0xC0UL | (codepoint >> 6)));
        out.push_back(static_cast<char>(0x80UL | (codepoint & 0x3FUL)));
    } else if (codepoint <= 0xFFFFUL) {
        out.push_back(static_cast<char>(0xE0UL | (codepoint >> 12)));
        out.push_back(static_cast<char>(0x80UL | ((codepoint >> 6) & 0x3FUL)));
        out.push_back(static_cast<char>(0x80UL | (codepoint & 0x3FUL)));
    } else {
        out.push_back(static_cast<char>(0xF0UL | (codepoint >> 18)));
        out.push_back(static_cast<char>(0x80UL | ((codepoint >> 12) & 0x3FUL)));
        out.push_back(static_cast<char>(0x80UL | ((codepoint >> 6) & 0x3FUL)));
        out.push_back(static_cast<char>(0x80UL | (codepoint & 0x3FUL)));
    }
}

double pow10_positive(int exponent)
{
    double result = 1.0;
    double base = 10.0;
    int exp = exponent;
    while (exp > 0) {
        if ((exp & 1) != 0) {
            result *= base;
        }
        base *= base;
        exp >>= 1;
    }
    return result;
}

// `literal` has already passed the JSON number grammar.
bool convert_literal(const std::string& literal, double& value)
{
    const char* text = literal.c_str();
    const std::size_t length = literal.size();
    std::size_t index = 0;
    int sign = 1;
    if (index < length && text[index] == '-') {
        sign = -1;
        ++index;
    }

    double significand = 0.0;
    int exponent = 0;
    int digits = 0;

    while (index < length && is_digit(text[index])) {
        const int digit = text[index] - '0';
        if (digits < 15) {
            significand = significand * 10.0 + static_cast<double>(digit);
            if (significand != 0.0) {
                ++digits;
            }
        } else {
            ++exponent;
        }
        ++index;
    }

    if (index < length && text[index] == '.') {
        ++index;
        while (index < length && is_digit(text[index])) {
            const int digit = text[index] - '0';
            if (digits < 15) {
                significand = significand * 10.0 + static_cast<double>(digit);
                --exponent;
                if (significand != 0.0) {
                    ++digits;
                }
            }
            ++index;
        }
    }

    if (index < length && (text[index] == 'e' || text[index] == 'E')) {
        ++index;
        int exponent_sign = 1;
        if (index < length && (text[index] == '+' || text[index] == '-')) {
            if (text[index] == '-') {
                exponent_sign = -1;
            }
            ++index;
        }
        int explicit_exponent = 0;
        int exponent_digits = 0;
        while (index < length && is_digit(text[index])) {
            const int digit = text[index] - '0';
            if (explicit_exponent < 100000000) {
                explicit_exponent = explicit_exponent * 10 + digit;
            }
            ++exponent_digits;
            ++index;
        }
        if (exponent_digits == 0) {
            return false;
        }
        if (exponent_sign < 0) {
            exponent -= explicit_exponent;
        } else {
            exponent += explicit_exponent;
        }
    }

    if (index != length) {
        return false;
    }
    if (significand == 0.0) {
        value = 0.0;
        return true;
    }
    if (exponent > 308) {
        return false;
    }
    if (exponent < -324) {
        value = 0.0;
        return true;
    }

    double magnitude = significand;
    if (exponent > 0) {
        magnitude *= pow10_positive(exponent);
    } else if (exponent < 0) {
        magnitude /= pow10_positive(-exponent);
    }
    if (magnitude != magnitude || magnitude == std::numeric_limits<double>::infinity()) {
        return false;
    }
    value = sign < 0 ? -magnitude : magnitude;
    return true;
}

void append_ulong(std::string& out, unsigned long value)
{
    char buffer[32];
    int count = 0;
    if (value == 0) {
        out.push_back('0');
        return;
    }
    while (value > 0 && count < 32) {
        buffer[count] = static_cast<char>('0' + static_cast<int>(value % 10UL));
        value /= 10UL;
        ++count;
    }
    while (count > 0) {
        --count;
        out.push_back(buffer[count]);
    }
}

void append_number(std::string& out, double value)
{
    if (value != value
        || value == std::numeric_limits<double>::infinity()
        || value == -std::numeric_limits<double>::infinity()) {
        out.append("null");
        return;
    }
    if (value == 0.0) {
        out.append("0");
        return;
    }

    const double absolute = value < 0.0 ? -value : value;
    const double truncated = std::floor(absolute);
    if (truncated == absolute && absolute <= 9007199254740992.0) {
        char buffer[64];
        const int count = std::snprintf(buffer, sizeof buffer, "%.0f", value);
        if (count > 0 && static_cast<std::size_t>(count) < sizeof buffer) {
            out.append(buffer);
            return;
        }
    }

    char buffer[64];
    const int count = std::snprintf(buffer, sizeof buffer, "%.17g", absolute);
    if (count <= 0 || static_cast<std::size_t>(count) >= sizeof buffer) {
        out.append("null");
        return;
    }
    if (value < 0.0) {
        out.push_back('-');
    }
    for (int i = 0; i < count; ++i) {
        char c = buffer[i];
        if (c == ',') {
            c = '.';
        }
        out.push_back(c);
    }
}

void append_string(std::string& out, const std::string& value)
{
    out.push_back('"');
    for (std::size_t i = 0; i < value.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(value[i]);
        switch (c) {
        case '"':
            out.append("\\\"");
            break;
        case '\\':
            out.append("\\\\");
            break;
        case '\b':
            out.append("\\b");
            break;
        case '\f':
            out.append("\\f");
            break;
        case '\n':
            out.append("\\n");
            break;
        case '\r':
            out.append("\\r");
            break;
        case '\t':
            out.append("\\t");
            break;
        default:
            if (c < 0x20) {
                char hex[8];
                std::snprintf(hex, sizeof hex, "\\u%04x", static_cast<unsigned int>(c));
                out.append(hex);
            } else {
                out.push_back(static_cast<char>(c));
            }
            break;
        }
    }
    out.push_back('"');
}

class DepthGuard {
public:
    explicit DepthGuard(int& depth)
        : depth_(depth)
    {
        ++depth_;
    }

    ~DepthGuard()
    {
        --depth_;
    }

private:
    int& depth_;
    DepthGuard(const DepthGuard&);
    DepthGuard& operator=(const DepthGuard&);
};

}

class JsonParser {
public:
    JsonParser(const char* text, std::size_t length)
        : begin_(text)
        , cur_(text)
        , end_(text == 0 ? 0 : text + length)
        , depth_(0)
        , failed_(false)
    {
    }

    bool parse(Json& out)
    {
        if (begin_ == 0) {
            return fail("empty input");
        }
        if (static_cast<std::size_t>(end_ - begin_) >= 3
            && static_cast<unsigned char>(begin_[0]) == 0xEF
            && static_cast<unsigned char>(begin_[1]) == 0xBB
            && static_cast<unsigned char>(begin_[2]) == 0xBF) {
            cur_ += 3;
        }
        if (!parse_value(out)) {
            return false;
        }
        skip_ws();
        if (cur_ != end_) {
            return fail("trailing data");
        }
        return true;
    }

    const Error& error() const
    {
        return error_;
    }

private:
    const char* begin_;
    const char* cur_;
    const char* end_;
    int depth_;
    bool failed_;
    Error error_;

    bool fail(const char* message)
    {
        if (!failed_) {
            failed_ = true;
            std::string text = message == 0 ? std::string("parse error") : std::string(message);
            text.append(" at byte ");
            unsigned long offset = 0;
            if (begin_ != 0 && cur_ != 0 && cur_ >= begin_) {
                offset = static_cast<unsigned long>(cur_ - begin_);
            }
            append_ulong(text, offset);
            error_.set(Error::kParseError, text.c_str());
        }
        return false;
    }

    void skip_ws()
    {
        while (cur_ < end_) {
            const char c = *cur_;
            if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
                break;
            }
            ++cur_;
        }
    }

    bool consume(char expected)
    {
        if (cur_ < end_ && *cur_ == expected) {
            ++cur_;
            return true;
        }
        return false;
    }

    bool parse_value(Json& out)
    {
        skip_ws();
        if (cur_ >= end_) {
            return fail("unexpected end");
        }
        const char c = *cur_;
        if (c == 'n') {
            return parse_literal("null", Json::null(), out);
        }
        if (c == 't') {
            return parse_literal("true", Json::boolean(true), out);
        }
        if (c == 'f') {
            return parse_literal("false", Json::boolean(false), out);
        }
        if (c == '"') {
            return parse_string_value(out);
        }
        if (c == '[') {
            return parse_array(out);
        }
        if (c == '{') {
            return parse_object(out);
        }
        if (c == '-' || is_digit(c)) {
            return parse_number(out);
        }
        return fail("unexpected character");
    }

    bool parse_literal(const char* keyword, const Json& value, Json& out)
    {
        const char* cursor = keyword;
        while (*cursor != '\0') {
            if (cur_ >= end_ || *cur_ != *cursor) {
                return fail("invalid literal");
            }
            ++cur_;
            ++cursor;
        }
        if (cur_ < end_) {
            const char next = *cur_;
            const bool identifier =
                (next >= 'a' && next <= 'z') ||
                (next >= 'A' && next <= 'Z') ||
                is_digit(next) ||
                next == '_';
            if (identifier) {
                return fail("invalid literal");
            }
        }
        out = value;
        return true;
    }

    bool parse_hex4(unsigned long& codepoint)
    {
        if (end_ - cur_ < 4) {
            return fail("bad unicode escape");
        }
        unsigned long value = 0;
        for (int i = 0; i < 4; ++i) {
            const int digit = hex_value(*cur_);
            if (digit < 0) {
                return fail("bad unicode escape");
            }
            value = (value << 4) | static_cast<unsigned long>(digit);
            ++cur_;
        }
        codepoint = value;
        return true;
    }

    bool parse_string_content(std::string& decoded)
    {
        if (!consume('"')) {
            return fail("expected string");
        }
        decoded.clear();
        while (cur_ < end_) {
            const unsigned char c = static_cast<unsigned char>(*cur_);
            if (c == '"') {
                ++cur_;
                return true;
            }
            if (c == '\\') {
                ++cur_;
                if (cur_ >= end_) {
                    return fail("unterminated string");
                }
                const char escaped = *cur_;
                ++cur_;
                switch (escaped) {
                case '"':
                case '\\':
                case '/':
                    decoded.push_back(escaped);
                    break;
                case 'b':
                    decoded.push_back('\b');
                    break;
                case 'f':
                    decoded.push_back('\f');
                    break;
                case 'n':
                    decoded.push_back('\n');
                    break;
                case 'r':
                    decoded.push_back('\r');
                    break;
                case 't':
                    decoded.push_back('\t');
                    break;
                case 'u': {
                    unsigned long codepoint = 0;
                    if (!parse_hex4(codepoint)) {
                        return false;
                    }
                    if (codepoint >= 0xD800UL && codepoint <= 0xDBFFUL) {
                        if (end_ - cur_ >= 2 && cur_[0] == '\\' && cur_[1] == 'u') {
                            cur_ += 2;
                            unsigned long low = 0;
                            if (!parse_hex4(low)) {
                                return false;
                            }
                            if (low >= 0xDC00UL && low <= 0xDFFFUL) {
                                const unsigned long pair =
                                    0x10000UL +
                                    (((codepoint - 0xD800UL) << 10) | (low - 0xDC00UL));
                                append_utf8(decoded, pair);
                            } else {
                                append_utf8(decoded, 0xFFFDUL);
                                append_utf8(decoded, low);
                            }
                        } else {
                            append_utf8(decoded, 0xFFFDUL);
                        }
                    } else {
                        append_utf8(decoded, codepoint);
                    }
                    break;
                }
                default:
                    return fail("bad escape");
                }
                continue;
            }
            if (c < 0x20) {
                return fail("unescaped control character");
            }
            decoded.push_back(static_cast<char>(c));
            ++cur_;
        }
        return fail("unterminated string");
    }

    bool parse_string_value(Json& out)
    {
        std::string decoded;
        if (!parse_string_content(decoded)) {
            return false;
        }
        out = Json::string(decoded);
        return true;
    }

    bool parse_number(Json& out)
    {
        const char* start = cur_;
        if (cur_ < end_ && *cur_ == '-') {
            ++cur_;
        }
        if (cur_ >= end_ || !is_digit(*cur_)) {
            return fail("invalid number");
        }
        if (*cur_ == '0') {
            ++cur_;
        } else {
            while (cur_ < end_ && is_digit(*cur_)) {
                ++cur_;
            }
        }
        if (cur_ < end_ && is_digit(*cur_)) {
            return fail("invalid number");
        }
        if (cur_ < end_ && *cur_ == '.') {
            ++cur_;
            if (cur_ >= end_ || !is_digit(*cur_)) {
                return fail("invalid number");
            }
            while (cur_ < end_ && is_digit(*cur_)) {
                ++cur_;
            }
        }
        if (cur_ < end_ && (*cur_ == 'e' || *cur_ == 'E')) {
            ++cur_;
            if (cur_ < end_ && (*cur_ == '+' || *cur_ == '-')) {
                ++cur_;
            }
            if (cur_ >= end_ || !is_digit(*cur_)) {
                return fail("invalid number");
            }
            while (cur_ < end_ && is_digit(*cur_)) {
                ++cur_;
            }
        }

        const std::string literal(start, cur_);
        double value = 0.0;
        if (!convert_literal(literal, value)) {
            return fail("number out of range");
        }
        out = Json::number(value);
        out.number_literal_ = literal;
        return true;
    }

    bool parse_array(Json& out)
    {
        if (depth_ >= kMaxDepth) {
            return fail("nesting limit exceeded");
        }
        DepthGuard guard(depth_);
        if (!consume('[')) {
            return fail("expected '['");
        }
        Json array = Json::array();
        skip_ws();
        if (consume(']')) {
            out = array;
            return true;
        }
        for (;;) {
            Json element;
            if (!parse_value(element)) {
                return false;
            }
            array.push_back(element);
            skip_ws();
            if (consume(']')) {
                out = array;
                return true;
            }
            if (!consume(',')) {
                return fail("expected ',' or ']'");
            }
            skip_ws();
        }
    }

    bool parse_object(Json& out)
    {
        if (depth_ >= kMaxDepth) {
            return fail("nesting limit exceeded");
        }
        DepthGuard guard(depth_);
        if (!consume('{')) {
            return fail("expected '{'");
        }
        Json object = Json::object();
        skip_ws();
        if (consume('}')) {
            out = object;
            return true;
        }
        for (;;) {
            skip_ws();
            std::string key;
            if (!parse_string_content(key)) {
                return false;
            }
            skip_ws();
            if (!consume(':')) {
                return fail("expected ':'");
            }
            Json field;
            if (!parse_value(field)) {
                return false;
            }
            object.set(key.c_str(), field);
            skip_ws();
            if (consume('}')) {
                out = object;
                return true;
            }
            if (!consume(',')) {
                return fail("expected ',' or '}'");
            }
        }
    }
};

Json::Json()
    : type_(kNull)
    , bool_(false)
    , number_(0.0)
    , number_literal_()
    , string_()
    , array_()
    , object_()
{
}

Json Json::null()
{
    return Json();
}

Json Json::boolean(bool value)
{
    Json json;
    json.type_ = kBool;
    json.bool_ = value;
    return json;
}

Json Json::number(double value)
{
    Json json;
    json.type_ = kNumber;
    json.number_ = value;
    return json;
}

Json Json::string(const char* value)
{
    Json json;
    json.type_ = kString;
    if (value != 0) {
        json.string_ = value;
    }
    return json;
}

Json Json::string(const std::string& value)
{
    Json json;
    json.type_ = kString;
    json.string_ = value;
    return json;
}

Json Json::array()
{
    Json json;
    json.type_ = kArray;
    return json;
}

Json Json::object()
{
    Json json;
    json.type_ = kObject;
    return json;
}

Json::Type Json::type() const
{
    return type_;
}

bool Json::is_null() const
{
    return type_ == kNull;
}

bool Json::is_bool() const
{
    return type_ == kBool;
}

bool Json::is_number() const
{
    return type_ == kNumber;
}

bool Json::is_string() const
{
    return type_ == kString;
}

bool Json::is_array() const
{
    return type_ == kArray;
}

bool Json::is_object() const
{
    return type_ == kObject;
}

bool Json::as_bool() const
{
    return type_ == kBool ? bool_ : false;
}

double Json::as_number() const
{
    return type_ == kNumber ? number_ : 0.0;
}

const char* Json::number_literal() const
{
    if (type_ != kNumber) {
        return "";
    }
    return number_literal_.c_str();
}

const std::string& Json::as_string() const
{
    if (type_ != kString) {
        static const std::string empty;
        return empty;
    }
    return string_;
}

std::size_t Json::size() const
{
    if (type_ == kArray) {
        return array_.size();
    }
    if (type_ == kObject) {
        return object_.size();
    }
    return 0;
}

bool Json::push_back(const Json& value)
{
    if (type_ != kArray) {
        return false;
    }
    // Copy first so `value` may alias storage inside this array.
    Json copy = value;
    array_.push_back(copy);
    return true;
}

const Json* Json::at(std::size_t index) const
{
    if (type_ != kArray || index >= array_.size()) {
        return 0;
    }
    return &array_[index];
}

Json* Json::at(std::size_t index)
{
    return const_cast<Json*>(static_cast<const Json*>(this)->at(index));
}

bool Json::set(const char* key, const Json& value)
{
    if (type_ != kObject || key == 0) {
        return false;
    }
    std::size_t found = object_.size();
    for (std::size_t i = 0; i < object_.size(); ++i) {
        if (object_[i].first == key) {
            found = i;
            break;
        }
    }
    Json copy = value;
    if (found < object_.size()) {
        object_[found].second = copy;
        return true;
    }
    object_.push_back(std::make_pair(std::string(key), copy));
    return true;
}

const Json* Json::find(const char* key) const
{
    if (type_ != kObject || key == 0) {
        return 0;
    }
    for (std::size_t i = 0; i < object_.size(); ++i) {
        if (object_[i].first == key) {
            return &object_[i].second;
        }
    }
    return 0;
}

Json* Json::find(const char* key)
{
    return const_cast<Json*>(static_cast<const Json*>(this)->find(key));
}

const std::string* Json::key_at(std::size_t index) const
{
    if (type_ != kObject || index >= object_.size()) {
        return 0;
    }
    return &object_[index].first;
}

void Json::write(std::string& out) const
{
    switch (type_) {
    case kNull:
        out.append("null");
        break;
    case kBool:
        out.append(bool_ ? "true" : "false");
        break;
    case kNumber:
        if (!number_literal_.empty()) {
            out.append(number_literal_);
        } else {
            append_number(out, number_);
        }
        break;
    case kString:
        append_string(out, string_);
        break;
    case kArray: {
        out.push_back('[');
        for (std::size_t i = 0; i < array_.size(); ++i) {
            if (i != 0) {
                out.push_back(',');
            }
            array_[i].write(out);
        }
        out.push_back(']');
        break;
    }
    case kObject: {
        out.push_back('{');
        for (std::size_t i = 0; i < object_.size(); ++i) {
            if (i != 0) {
                out.push_back(',');
            }
            append_string(out, object_[i].first);
            out.push_back(':');
            object_[i].second.write(out);
        }
        out.push_back('}');
        break;
    }
    }
}

std::string Json::dump() const
{
    std::string out;
    write(out);
    return out;
}

bool Json::parse(const char* text, std::size_t length, Json& out, Error& error)
{
    JsonParser parser(text, length);
    Json parsed;
    if (!parser.parse(parsed)) {
        out = Json();
        error = parser.error();
        if (!error.failed()) {
            error.set(Error::kParseError, "parse error");
        }
        return false;
    }
    out = parsed;
    error.clear();
    return true;
}

bool Json::parse(const std::string& text, Json& out, Error& error)
{
    return parse(text.c_str(), text.size(), out, error);
}

}
