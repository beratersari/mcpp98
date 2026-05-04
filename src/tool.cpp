#include "mcp/tool.hpp"

namespace mcp {
namespace {

Json typed_property(const char* type, const char* description)
{
    Json property = Json::object();
    property.set("type", Json::string(type == 0 ? "" : type));
    if (description != 0 && description[0] != '\0') {
        property.set("description", Json::string(description));
    }
    return property;
}

}

Json object_schema()
{
    Json schema = Json::object();
    schema.set("type", Json::string("object"));
    schema.set("properties", Json::object());
    return schema;
}

Json schema_string(const char* description)
{
    return typed_property("string", description);
}

Json schema_number(const char* description)
{
    return typed_property("number", description);
}

Json schema_boolean(const char* description)
{
    return typed_property("boolean", description);
}

void schema_add(Json& schema, const char* name, const Json& property, bool required)
{
    if (!schema.is_object() || name == 0) {
        return;
    }
    Json* properties = schema.find("properties");
    if (properties == 0 || !properties->is_object()) {
        schema.set("properties", Json::object());
        properties = schema.find("properties");
    }
    if (properties == 0) {
        return;
    }
    properties->set(name, property);
    if (!required) {
        return;
    }
    Json* required_names = schema.find("required");
    if (required_names == 0 || !required_names->is_array()) {
        schema.set("required", Json::array());
        required_names = schema.find("required");
    }
    if (required_names != 0) {
        required_names->push_back(Json::string(name));
    }
}

Json text_result(const std::string& text)
{
    Json item = Json::object();
    item.set("type", Json::string("text"));
    item.set("text", Json::string(text));

    Json content = Json::array();
    content.push_back(item);

    Json result = Json::object();
    result.set("content", content);
    return result;
}

Json error_result(const std::string& text)
{
    Json result = text_result(text);
    result.set("isError", Json::boolean(true));
    return result;
}

}
