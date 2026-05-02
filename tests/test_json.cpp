#include "harness.hpp"

#include "mcp/json.hpp"

#include <string>

namespace {

bool parse_ok(const char* text, mcp::Json& out)
{
    mcp::Error error;
    const bool ok = mcp::Json::parse(text, out, error);
    return ok && !error.failed();
}

}

int test_json()
{
    TestContext ctx;
    ctx.failed = 0;
    ctx.suite = "json";

    mcp::Json value;
    mcp::Error error;

    CHECK(parse_ok("null", value));
    CHECK(value.is_null());
    CHECK(value.dump() == "null");

    CHECK(parse_ok("true", value));
    CHECK(value.is_bool());
    CHECK(value.as_bool());
    CHECK(parse_ok("false", value));
    CHECK(!value.as_bool());

    CHECK(parse_ok("0", value));
    CHECK(value.is_number());
    CHECK(value.as_number() == 0.0);
    CHECK(std::string(value.number_literal()) == "0");

    CHECK(parse_ok("-32700", value));
    CHECK(value.as_number() == -32700.0);
    CHECK(value.dump() == "-32700");

    CHECK(parse_ok("1.5", value));
    CHECK(value.as_number() == 1.5);
    CHECK(value.dump() == "1.5");

    CHECK(parse_ok("1e2", value));
    CHECK(value.as_number() == 100.0);
    CHECK(std::string(value.number_literal()) == "1e2");
    CHECK(value.dump() == "1e2");

    CHECK(parse_ok("0.001", value));
    CHECK(value.as_number() > 0.0009 && value.as_number() < 0.0011);

    CHECK(!mcp::Json::parse("01", value, error));
    CHECK(error.code() == mcp::Error::kParseError);
    CHECK(!mcp::Json::parse("1.", value, error));
    CHECK(!mcp::Json::parse("[1,]", value, error));
    CHECK(!mcp::Json::parse("{\"a\"}", value, error));
    CHECK(!mcp::Json::parse("", value, error));
    CHECK(!mcp::Json::parse("{", value, error));

    CHECK(parse_ok("\"say \\\"hi\\\"\"", value));
    CHECK(value.as_string() == "say \"hi\"");

    CHECK(parse_ok("\"a\\nb\"", value));
    CHECK(value.as_string().size() == 3);
    CHECK(value.as_string()[1] == '\n');
    CHECK(value.dump().find('\n') == std::string::npos);
    CHECK(value.dump() == "\"a\\nb\"");

    CHECK(parse_ok("\"\\u00e9\"", value));
    CHECK(value.as_string() == "\xC3\xA9");

    CHECK(parse_ok("\"\\uD83D\\uDE00\"", value));
    CHECK(value.as_string() == "\xF0\x9F\x98\x80");

    CHECK(parse_ok("\"\\uD800\"", value));
    CHECK(value.as_string() == "\xEF\xBF\xBD");

    CHECK(parse_ok("[1, 2, 3]", value));
    CHECK(value.is_array());
    CHECK(value.size() == 3);
    CHECK(value.at(1) != 0);
    CHECK(value.at(1)->as_number() == 2.0);
    CHECK(value.at(3) == 0);

    CHECK(parse_ok("{\"b\": true, \"a\": null}", value));
    CHECK(value.size() == 2);
    CHECK(value.key_at(0) != 0);
    CHECK(*value.key_at(0) == "b");
    CHECK(value.find("a") != 0);
    CHECK(value.find("a")->is_null());
    CHECK(value.find("missing") == 0);

    CHECK(parse_ok("{\"a\":1,\"a\":2}", value));
    CHECK(value.size() == 1);
    CHECK(value.find("a")->as_number() == 2.0);

    mcp::Json nested;
    CHECK(parse_ok("{\"tools\":[{\"name\":\"echo\"}]}", nested));
    CHECK(nested.find("tools") != 0);
    CHECK(nested.find("tools")->at(0)->find("name")->as_string() == "echo");

    mcp::Json made = mcp::Json::number(-32700);
    CHECK(made.dump() == "-32700");
    mcp::Json one = mcp::Json::number(1);
    CHECK(one.dump() == "1");
    mcp::Json half = mcp::Json::number(1.5);
    mcp::Json parsed_half;
    CHECK(parse_ok(half.dump().c_str(), parsed_half));
    CHECK(parsed_half.as_number() == 1.5);

    mcp::Json array = mcp::Json::array();
    CHECK(array.push_back(mcp::Json::number(1)));
    CHECK(array.push_back(array));
    CHECK(array.size() == 2);
    CHECK(array.at(1) != 0);
    CHECK(array.at(1)->is_array());
    CHECK(array.at(1)->size() == 1);

    mcp::Json object = mcp::Json::object();
    CHECK(object.set("a", mcp::Json::string("x")));
    CHECK(object.set("b", mcp::Json::boolean(true)));
    CHECK(object.key_at(0) != 0 && *object.key_at(0) == "a");
    CHECK(object.key_at(1) != 0 && *object.key_at(1) == "b");
    CHECK(!object.push_back(mcp::Json::null()));
    CHECK(!mcp::Json::array().set("a", mcp::Json::null()));

    std::string deep;
    for (int i = 0; i < 64; ++i) {
        deep.push_back('[');
    }
    deep.append("null");
    for (int i = 0; i < 64; ++i) {
        deep.push_back(']');
    }
    CHECK(parse_ok(deep.c_str(), value));
    deep.push_back(']');
    deep.insert(deep.begin(), '[');
    CHECK(!mcp::Json::parse(deep, value, error));

    CHECK(parse_ok("  { \"n\" : 1 }  ", value));
    CHECK(value.find("n")->dump() == "1");

    return ctx.failed;
}
