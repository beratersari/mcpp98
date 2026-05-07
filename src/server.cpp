#include "mcp/server.hpp"

#include "mcp/rpc.hpp"
#include "mcp/session.hpp"

#include <cstring>
#include <exception>
#include <vector>

namespace mcp {

struct Server::Impl : ServerContext {
    Impl(const char* name, const char* version)
        : name_(name == 0 ? std::string() : std::string(name))
        , version_(version == 0 ? std::string() : std::string(version))
        , instructions_()
        , tools_()
        , session_(*this)
    {
    }

    void set_instructions(const char* text)
    {
        if (text == 0) {
            instructions_.clear();
        } else {
            instructions_ = text;
        }
    }

    bool add_tool(Tool* tool)
    {
        if (tool == 0 || tool->name() == 0 || tool->name()[0] == '\0') {
            return false;
        }
        const char* name = tool->name();
        for (std::size_t i = 0; i < tools_.size(); ++i) {
            const char* existing = tools_[i]->name();
            if (existing != 0 && std::strcmp(existing, name) == 0) {
                return false;
            }
        }
        tools_.push_back(tool);
        return true;
    }

    void handle_line(const std::string& line, std::string& response, bool& has_response)
    {
        session_.handle_line(line, response, has_response);
    }

    virtual const std::string& server_name() const
    {
        return name_;
    }

    virtual const std::string& server_version() const
    {
        return version_;
    }

    virtual const std::string& instructions() const
    {
        return instructions_;
    }

    virtual void list_tools(Json& tools) const
    {
        tools = Json::array();
        for (std::size_t i = 0; i < tools_.size(); ++i) {
            Tool* tool = tools_[i];
            if (tool == 0 || tool->name() == 0) {
                continue;
            }
            Json item = Json::object();
            item.set("name", Json::string(tool->name()));
            const char* description = tool->description();
            if (description != 0 && description[0] != '\0') {
                item.set("description", Json::string(description));
            }
            Json schema = tool->input_schema();
            if (!schema.is_object()) {
                schema = Json::object();
                schema.set("type", Json::string("object"));
            }
            item.set("inputSchema", schema);
            tools.push_back(item);
        }
    }

    virtual bool call_tool(const std::string& name,
                           const Json& arguments,
                           Json& result,
                           Error& error)
    {
        Tool* found = 0;
        for (std::size_t i = 0; i < tools_.size(); ++i) {
            const char* tool_name = tools_[i]->name();
            if (tool_name != 0 && name == tool_name) {
                found = tools_[i];
                break;
            }
        }
        if (found == 0) {
            error.set(Error::kInvalidParams, "unknown tool");
            return false;
        }
#if defined(__EXCEPTIONS)
        try {
            found->call(arguments, result, error);
        } catch (const std::exception& ex) {
            const char* what = ex.what();
            error.set(Error::kInternalError, what == 0 ? "tool threw" : what);
        } catch (...) {
            error.set(Error::kInternalError, "tool threw");
        }
#else
        found->call(arguments, result, error);
#endif
        return true;
    }

    std::string name_;
    std::string version_;
    std::string instructions_;
    std::vector<Tool*> tools_;
    Session session_;
};

Server::Server(const char* name, const char* version)
    : impl_(new Impl(name, version))
{
}

Server::~Server()
{
    delete impl_;
    impl_ = 0;
}

void Server::set_instructions(const char* text)
{
    if (impl_ != 0) {
        impl_->set_instructions(text);
    }
}

bool Server::add_tool(Tool* tool)
{
    if (impl_ == 0) {
        return false;
    }
    return impl_->add_tool(tool);
}

void Server::handle_line(const std::string& line, std::string& response, bool& has_response)
{
    if (impl_ == 0) {
        response.clear();
        has_response = false;
        return;
    }
    impl_->handle_line(line, response, has_response);
}

int Server::run(Transport& transport)
{
    if (impl_ == 0) {
        return 1;
    }
    std::string line;
    std::string response;
    bool has_response = false;
    for (;;) {
        const Transport::ReadStatus status = transport.read_line(line);
        if (status == Transport::kReadEof) {
            return 0;
        }
        if (status == Transport::kReadError) {
            return 1;
        }
        if (status == Transport::kReadTooLong) {
            Error error(Error::kParseError, "message exceeds size limit");
            response = RpcMessage::error_message(Json::null(), error).dump();
            if (!transport.write_line(response)) {
                return 1;
            }
            continue;
        }
        impl_->handle_line(line, response, has_response);
        if (has_response && !transport.write_line(response)) {
            return 1;
        }
    }
}

int Server::run_stdio()
{
    StdioTransport transport;
    return run(transport);
}

}
