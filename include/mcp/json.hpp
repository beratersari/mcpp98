#ifndef MCP_JSON_HPP
#define MCP_JSON_HPP

#include "mcp/error.hpp"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace mcp {

class JsonParser;

// JSON value. Objects keep insertion order. Numbers parsed from text keep
// their original literal so a JSON-RPC id can be echoed unchanged.
// Pointers returned by find/at/key_at are invalidated by any mutation.
class Json {
public:
    enum Type {
        kNull,
        kBool,
        kNumber,
        kString,
        kArray,
        kObject
    };

    Json();

    static Json null();
    static Json boolean(bool value);
    static Json number(double value);
    static Json string(const char* value);
    static Json string(const std::string& value);
    static Json array();
    static Json object();

    Type type() const;
    bool is_null() const;
    bool is_bool() const;
    bool is_number() const;
    bool is_string() const;
    bool is_array() const;
    bool is_object() const;

    bool as_bool() const;
    double as_number() const;
    // Empty when the number was built with Json::number rather than parsed.
    const char* number_literal() const;
    const std::string& as_string() const;

    // Array length, object member count, or 0 for every other type.
    std::size_t size() const;

    bool push_back(const Json& value);
    const Json* at(std::size_t index) const;
    Json* at(std::size_t index);

    bool set(const char* key, const Json& value);
    const Json* find(const char* key) const;
    Json* find(const char* key);
    const std::string* key_at(std::size_t index) const;

    std::string dump() const;

    static bool parse(const char* text, std::size_t length, Json& out, Error& error);
    static bool parse(const std::string& text, Json& out, Error& error);

private:
    friend class JsonParser;

    void write(std::string& out) const;

    Type type_;
    bool bool_;
    double number_;
    std::string number_literal_;
    std::string string_;
    std::vector<Json> array_;
    std::vector<std::pair<std::string, Json> > object_;
};

}

#endif
