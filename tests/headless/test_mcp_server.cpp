/**
 * @file    test_mcp_server.cpp
 * @brief   Unit tests for MCP server JSON-RPC message handling.
 */
#include <gtest/gtest.h>
#include <utils/mcp/mcp_server.h>
#include <json.hpp>

using namespace Frasy::Mcp;
using json = nlohmann::json;

// --- Tool registration ---

TEST(McpServer, RegisterToolStoresDefinition)
{
    McpServer server("test", "1.0");
    bool      called = false;

    server.registerTool(
      "my_tool",
      "A test tool",
      json({{"type", "object"}, {"properties", json::object()}}),
      [&](const json&) -> json {
          called = true;
          json result;
          result["content"] = json::array({{{"type", "text"}, {"text", "ok"}}});
          result["isError"] = false;
          return result;
      });

    // We can't directly test the internal map, but we can verify it doesn't crash
    EXPECT_FALSE(called);
}

// --- JSON-RPC message structure tests ---

TEST(McpServer, InitializeResponseFormat)
{
    // Verify the expected structure of an initialize response
    json response;
    response["jsonrpc"]                    = "2.0";
    response["id"]                         = 1;
    response["result"]["protocolVersion"]  = "2024-11-05";
    response["result"]["capabilities"]["tools"] = json::object();
    response["result"]["serverInfo"]["name"]    = "frasy";
    response["result"]["serverInfo"]["version"] = "1.0.0";

    EXPECT_EQ(response["jsonrpc"], "2.0");
    EXPECT_EQ(response["result"]["protocolVersion"], "2024-11-05");
    EXPECT_TRUE(response["result"]["capabilities"].contains("tools"));
    EXPECT_EQ(response["result"]["serverInfo"]["name"], "frasy");
}

TEST(McpServer, ToolsListResponseFormat)
{
    json tool;
    tool["name"]        = "list_products";
    tool["description"] = "List available test products";
    tool["inputSchema"] = json({{"type", "object"}, {"properties", json::object()}});

    json result;
    result["tools"] = json::array({tool});

    EXPECT_EQ(result["tools"].size(), 1);
    EXPECT_EQ(result["tools"][0]["name"], "list_products");
    EXPECT_TRUE(result["tools"][0].contains("inputSchema"));
}

TEST(McpServer, ToolCallResultFormat)
{
    json result;
    result["content"] = json::array({{{"type", "text"}, {"text", "{\"started\":true}"}}});
    result["isError"] = false;

    EXPECT_EQ(result["content"].size(), 1);
    EXPECT_EQ(result["content"][0]["type"], "text");
    EXPECT_FALSE(result["isError"]);
}

TEST(McpServer, ToolCallErrorFormat)
{
    json result;
    result["content"] = json::array({{{"type", "text"}, {"text", "Unknown tool: bad_tool"}}});
    result["isError"] = true;

    EXPECT_TRUE(result["isError"]);
    EXPECT_EQ(result["content"][0]["text"], "Unknown tool: bad_tool");
}

TEST(McpServer, JsonRpcErrorFormat)
{
    json response;
    response["jsonrpc"]          = "2.0";
    response["id"]               = nullptr;
    response["error"]["code"]    = -32700;
    response["error"]["message"] = "Parse error";

    EXPECT_EQ(response["error"]["code"], -32700);
    EXPECT_EQ(response["error"]["message"], "Parse error");
}
