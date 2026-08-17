/**
 * @file    test.cpp
 * @brief   Unit tests for Frasy::CliArgs parser.
 */
#include <gtest/gtest.h>
#include <utils/cli/cli_args.h>

namespace {

/// Helper to build a fake argc/argv from a vector of strings.
/// The returned argv pointers remain valid as long as `storage` is alive.
struct ArgvBuilder {
    std::vector<std::string> storage;
    std::vector<char*>       ptrs;

    ArgvBuilder(std::initializer_list<std::string> args)
    {
        storage.reserve(args.size());
        for (const auto& a : args) {
            storage.push_back(a);
        }
        ptrs.reserve(storage.size());
        for (auto& s : storage) {
            ptrs.push_back(s.data());
        }
    }

    int    argc() const { return static_cast<int>(ptrs.size()); }
    char** argv() { return ptrs.data(); }
};

}    // namespace

// --- Basic flag parsing ---

TEST(CliArgs, NoArgsReturnsDefaults)
{
    ArgvBuilder ab {"frasy.exe"};
    auto        args = Frasy::CliArgs::parse(ab.argc(), ab.argv());

    EXPECT_FALSE(args.headless);
    EXPECT_TRUE(args.product.empty());
    EXPECT_TRUE(args.operatorName.empty());
    EXPECT_TRUE(args.serials.empty());
    EXPECT_EQ(args.configPath, "config.json");
    EXPECT_EQ(args.outputFormat, "human");
    EXPECT_EQ(args.outputDir, "logs");
    EXPECT_FALSE(args.skipVerification);
    EXPECT_EQ(args.popupTimeoutSeconds, 0);
}

TEST(CliArgs, HeadlessFlagParsed)
{
    ArgvBuilder ab {"frasy.exe", "--headless", "--product", "MyProduct", "--operator", "CI", "--serial", "SN001"};
    auto        args = Frasy::CliArgs::parse(ab.argc(), ab.argv());

    EXPECT_TRUE(args.headless);
}

TEST(CliArgs, ProductParsed)
{
    ArgvBuilder ab {"frasy.exe", "--headless", "--product", "TestProduct", "--operator", "Op", "--serial", "SN1"};
    auto        args = Frasy::CliArgs::parse(ab.argc(), ab.argv());

    EXPECT_EQ(args.product, "TestProduct");
}

TEST(CliArgs, OperatorParsed)
{
    ArgvBuilder ab {"frasy.exe", "--headless", "--product", "P", "--operator", "John Doe", "--serial", "SN1"};
    auto        args = Frasy::CliArgs::parse(ab.argc(), ab.argv());

    EXPECT_EQ(args.operatorName, "John Doe");
}

TEST(CliArgs, SingleSerialParsed)
{
    ArgvBuilder ab {"frasy.exe", "--headless", "--product", "P", "--operator", "Op", "--serial", "SN001"};
    auto        args = Frasy::CliArgs::parse(ab.argc(), ab.argv());

    ASSERT_EQ(args.serials.size(), 1);
    EXPECT_EQ(args.serials[0], "SN001");
}

TEST(CliArgs, MultipleSerialsParsed)
{
    ArgvBuilder ab {"frasy.exe", "--headless", "--product", "P",      "--operator", "Op",
                    "--serial",  "SN001",     "--serial",  "SN002",   "--serial",   "SN003"};
    auto        args = Frasy::CliArgs::parse(ab.argc(), ab.argv());

    ASSERT_EQ(args.serials.size(), 3);
    EXPECT_EQ(args.serials[0], "SN001");
    EXPECT_EQ(args.serials[1], "SN002");
    EXPECT_EQ(args.serials[2], "SN003");
}

TEST(CliArgs, ConfigPathParsed)
{
    ArgvBuilder ab {"frasy.exe", "--config", "custom_config.json"};
    auto        args = Frasy::CliArgs::parse(ab.argc(), ab.argv());

    EXPECT_EQ(args.configPath, "custom_config.json");
}

TEST(CliArgs, OutputFormatJsonParsed)
{
    ArgvBuilder ab {"frasy.exe",   "--headless", "--product", "P",    "--operator",
                    "Op",          "--serial",   "SN1",       "--output-format", "json"};
    auto        args = Frasy::CliArgs::parse(ab.argc(), ab.argv());

    EXPECT_EQ(args.outputFormat, "json");
}

TEST(CliArgs, OutputFormatHumanParsed)
{
    ArgvBuilder ab {"frasy.exe",   "--headless", "--product", "P",    "--operator",
                    "Op",          "--serial",   "SN1",       "--output-format", "human"};
    auto        args = Frasy::CliArgs::parse(ab.argc(), ab.argv());

    EXPECT_EQ(args.outputFormat, "human");
}

TEST(CliArgs, OutputDirParsed)
{
    ArgvBuilder ab {"frasy.exe",   "--headless", "--product", "P", "--operator",
                    "Op",          "--serial",   "SN1",       "--output-dir", "my_output"};
    auto        args = Frasy::CliArgs::parse(ab.argc(), ab.argv());

    EXPECT_EQ(args.outputDir, "my_output");
}

TEST(CliArgs, SkipVerificationParsed)
{
    ArgvBuilder ab {"frasy.exe", "--headless",          "--product", "P", "--operator",
                    "Op",        "--serial",            "SN1",       "--skip-verification"};
    auto        args = Frasy::CliArgs::parse(ab.argc(), ab.argv());

    EXPECT_TRUE(args.skipVerification);
}

TEST(CliArgs, PopupTimeoutParsed)
{
    ArgvBuilder ab {"frasy.exe",   "--headless", "--product", "P", "--operator",
                    "Op",          "--serial",   "SN1",       "--popup-timeout", "30"};
    auto        args = Frasy::CliArgs::parse(ab.argc(), ab.argv());

    EXPECT_EQ(args.popupTimeoutSeconds, 30);
}

TEST(CliArgs, PopupTimeoutZeroParsed)
{
    ArgvBuilder ab {"frasy.exe",   "--headless", "--product", "P", "--operator",
                    "Op",          "--serial",   "SN1",       "--popup-timeout", "0"};
    auto        args = Frasy::CliArgs::parse(ab.argc(), ab.argv());

    EXPECT_EQ(args.popupTimeoutSeconds, 0);
}

// --- Non-headless mode ignores extra flags ---

TEST(CliArgs, NonHeadlessModeIgnoresFlags)
{
    ArgvBuilder ab {"frasy.exe", "--product", "P", "--operator", "Op", "--serial", "SN1"};
    auto        args = Frasy::CliArgs::parse(ab.argc(), ab.argv());

    EXPECT_FALSE(args.headless);
    // Flags are still parsed/stored, just no validation error
    EXPECT_EQ(args.product, "P");
    EXPECT_EQ(args.operatorName, "Op");
    ASSERT_EQ(args.serials.size(), 1);
}

// --- get() returns the last parsed result ---

TEST(CliArgs, GetReturnsLastParsedResult)
{
    ArgvBuilder ab {"frasy.exe", "--headless", "--product", "GetTest", "--operator", "Op", "--serial", "SN1"};
    Frasy::CliArgs::parse(ab.argc(), ab.argv());

    const auto& retrieved = Frasy::CliArgs::get();
    EXPECT_TRUE(retrieved.headless);
    EXPECT_EQ(retrieved.product, "GetTest");
}

// --- All flags combined ---

TEST(CliArgs, AllFlagsCombined)
{
    ArgvBuilder ab {"frasy.exe",         "--headless",
                    "--product",         "FullTest",
                    "--operator",        "Alice",
                    "--serial",          "SN001",
                    "--serial",          "SN002",
                    "--config",          "alt.json",
                    "--output-format",   "json",
                    "--output-dir",      "results",
                    "--skip-verification",
                    "--popup-timeout",   "60"};
    auto        args = Frasy::CliArgs::parse(ab.argc(), ab.argv());

    EXPECT_TRUE(args.headless);
    EXPECT_EQ(args.product, "FullTest");
    EXPECT_EQ(args.operatorName, "Alice");
    ASSERT_EQ(args.serials.size(), 2);
    EXPECT_EQ(args.serials[0], "SN001");
    EXPECT_EQ(args.serials[1], "SN002");
    EXPECT_EQ(args.configPath, "alt.json");
    EXPECT_EQ(args.outputFormat, "json");
    EXPECT_EQ(args.outputDir, "results");
    EXPECT_TRUE(args.skipVerification);
    EXPECT_EQ(args.popupTimeoutSeconds, 60);
}

// --- Validation tests (headless mode missing required flags) ---
// These call std::exit(2) which we test via death tests.
// Note: We use helper functions to avoid MSVC preprocessor issues with commas in macro args.

namespace {
void parseMissingProduct()
{
    ArgvBuilder ab {"frasy.exe", "--headless", "--operator", "Op", "--serial", "SN1"};
    Frasy::CliArgs::parse(ab.argc(), ab.argv());
}

void parseMissingOperator()
{
    ArgvBuilder ab {"frasy.exe", "--headless", "--product", "P", "--serial", "SN1"};
    Frasy::CliArgs::parse(ab.argc(), ab.argv());
}

void parseMissingSerial()
{
    ArgvBuilder ab {"frasy.exe", "--headless", "--product", "P", "--operator", "Op"};
    Frasy::CliArgs::parse(ab.argc(), ab.argv());
}

void parseInvalidOutputFormat()
{
    ArgvBuilder ab {"frasy.exe", "--output-format", "xml"};
    Frasy::CliArgs::parse(ab.argc(), ab.argv());
}

void parseInvalidPopupTimeout()
{
    ArgvBuilder ab {"frasy.exe", "--popup-timeout", "abc"};
    Frasy::CliArgs::parse(ab.argc(), ab.argv());
}

void parseNegativePopupTimeout()
{
    ArgvBuilder ab {"frasy.exe", "--popup-timeout", "-5"};
    Frasy::CliArgs::parse(ab.argc(), ab.argv());
}

void parseMissingValueForProduct()
{
    ArgvBuilder ab {"frasy.exe", "--product"};
    Frasy::CliArgs::parse(ab.argc(), ab.argv());
}

void parseHelp()
{
    ArgvBuilder ab {"frasy.exe", "--help"};
    Frasy::CliArgs::parse(ab.argc(), ab.argv());
}
}    // namespace

TEST(CliArgsDeathTest, HeadlessMissingProductExits)
{
    EXPECT_EXIT(parseMissingProduct(), ::testing::ExitedWithCode(2), ".*--product is required.*");
}

TEST(CliArgsDeathTest, HeadlessMissingOperatorExits)
{
    EXPECT_EXIT(parseMissingOperator(), ::testing::ExitedWithCode(2), ".*--operator is required.*");
}

TEST(CliArgsDeathTest, HeadlessMissingSerialExits)
{
    EXPECT_EXIT(parseMissingSerial(), ::testing::ExitedWithCode(2), ".*--serial is required.*");
}

TEST(CliArgsDeathTest, InvalidOutputFormatExits)
{
    EXPECT_EXIT(parseInvalidOutputFormat(), ::testing::ExitedWithCode(2), ".*must be 'human' or 'json'.*");
}

TEST(CliArgsDeathTest, InvalidPopupTimeoutExits)
{
    EXPECT_EXIT(parseInvalidPopupTimeout(), ::testing::ExitedWithCode(2), ".*must be a valid integer.*");
}

TEST(CliArgsDeathTest, NegativePopupTimeoutExits)
{
    EXPECT_EXIT(parseNegativePopupTimeout(), ::testing::ExitedWithCode(2), ".*must be a non-negative integer.*");
}

TEST(CliArgsDeathTest, MissingValueForProductExits)
{
    EXPECT_EXIT(parseMissingValueForProduct(), ::testing::ExitedWithCode(2), ".*requires a value.*");
}

TEST(CliArgsDeathTest, HelpExitsZero)
{
    EXPECT_EXIT(parseHelp(), ::testing::ExitedWithCode(0), "");
}

// --- Unknown flags are silently ignored ---

TEST(CliArgs, UnknownFlagsIgnored)
{
    ArgvBuilder ab {"frasy.exe",   "--headless",     "--product", "P", "--operator",
                    "Op",          "--serial",       "SN1",       "--unknown-flag", "value",
                    "--another"};
    auto        args = Frasy::CliArgs::parse(ab.argc(), ab.argv());

    EXPECT_TRUE(args.headless);
    EXPECT_EQ(args.product, "P");
}

// --- MCP port flag ---

TEST(CliArgs, McpPortParsed)
{
    ArgvBuilder ab {"frasy.exe", "--mcp-port", "8080"};
    auto        args = Frasy::CliArgs::parse(ab.argc(), ab.argv());

    EXPECT_EQ(args.mcpPort, 8080);
}

TEST(CliArgs, McpPortZeroMeansAuto)
{
    ArgvBuilder ab {"frasy.exe", "--mcp-port", "0"};
    auto        args = Frasy::CliArgs::parse(ab.argc(), ab.argv());

    EXPECT_EQ(args.mcpPort, 0);
}

TEST(CliArgs, McpPortDefaultIsDisabled)
{
    ArgvBuilder ab {"frasy.exe"};
    auto        args = Frasy::CliArgs::parse(ab.argc(), ab.argv());

    EXPECT_EQ(args.mcpPort, -1);
}

// --- MCP client flag ---

TEST(CliArgs, McpClientParsed)
{
    ArgvBuilder ab {"frasy.exe", "--mcp-client", "--port", "8080"};
    auto        args = Frasy::CliArgs::parse(ab.argc(), ab.argv());

    EXPECT_TRUE(args.mcpClient);
    EXPECT_EQ(args.port, 8080);
}

TEST(CliArgs, McpClientWithAddress)
{
    ArgvBuilder ab {"frasy.exe", "--mcp-client", "--address", "192.168.0.105", "--port", "69"};
    auto        args = Frasy::CliArgs::parse(ab.argc(), ab.argv());

    EXPECT_TRUE(args.mcpClient);
    EXPECT_EQ(args.address, "192.168.0.105");
    EXPECT_EQ(args.port, 69);
}

TEST(CliArgs, McpClientDefaultAddress)
{
    ArgvBuilder ab {"frasy.exe", "--mcp-client", "--port", "8080"};
    auto        args = Frasy::CliArgs::parse(ab.argc(), ab.argv());

    EXPECT_EQ(args.address, "127.0.0.1");
}

// --- Address and port without mcp-client ---

TEST(CliArgs, AddressParsedAlone)
{
    ArgvBuilder ab {"frasy.exe", "--address", "10.0.0.1"};
    auto        args = Frasy::CliArgs::parse(ab.argc(), ab.argv());

    EXPECT_EQ(args.address, "10.0.0.1");
}

TEST(CliArgs, PortParsedAlone)
{
    // --port alone without --mcp-client won't trigger mutual exclusivity error
    // but will still be parsed
    ArgvBuilder ab {"frasy.exe", "--port", "1234"};
    auto        args = Frasy::CliArgs::parse(ab.argc(), ab.argv());

    EXPECT_EQ(args.port, 1234);
}

// --- Mutual exclusivity death tests ---

namespace {
void parseMcpClientWithoutPort()
{
    ArgvBuilder ab {"frasy.exe", "--mcp-client"};
    Frasy::CliArgs::parse(ab.argc(), ab.argv());
}

void parseMcpClientWithHeadless()
{
    ArgvBuilder ab {"frasy.exe", "--mcp-client", "--port", "80", "--headless", "--product", "P", "--operator", "Op", "--serial", "SN"};
    Frasy::CliArgs::parse(ab.argc(), ab.argv());
}

void parseMcpClientWithMcpServer()
{
    ArgvBuilder ab {"frasy.exe", "--mcp-client", "--port", "80", "--mcp-server"};
    Frasy::CliArgs::parse(ab.argc(), ab.argv());
}

void parseMcpClientWithMcpPort()
{
    ArgvBuilder ab {"frasy.exe", "--mcp-client", "--port", "80", "--mcp-port", "9090"};
    Frasy::CliArgs::parse(ab.argc(), ab.argv());
}

void parseMcpPortWithHeadless()
{
    ArgvBuilder ab {"frasy.exe", "--mcp-port", "8080", "--headless", "--product", "P", "--operator", "Op", "--serial", "SN"};
    Frasy::CliArgs::parse(ab.argc(), ab.argv());
}

void parseMcpPortWithMcpServer()
{
    ArgvBuilder ab {"frasy.exe", "--mcp-port", "8080", "--mcp-server"};
    Frasy::CliArgs::parse(ab.argc(), ab.argv());
}

void parseInvalidMcpPort()
{
    ArgvBuilder ab {"frasy.exe", "--mcp-port", "abc"};
    Frasy::CliArgs::parse(ab.argc(), ab.argv());
}

void parseNegativeMcpPort()
{
    ArgvBuilder ab {"frasy.exe", "--mcp-port", "-5"};
    Frasy::CliArgs::parse(ab.argc(), ab.argv());
}

void parseInvalidPort()
{
    ArgvBuilder ab {"frasy.exe", "--port", "abc"};
    Frasy::CliArgs::parse(ab.argc(), ab.argv());
}

void parseZeroPort()
{
    ArgvBuilder ab {"frasy.exe", "--port", "0"};
    Frasy::CliArgs::parse(ab.argc(), ab.argv());
}

void parseNegativePort()
{
    ArgvBuilder ab {"frasy.exe", "--port", "-1"};
    Frasy::CliArgs::parse(ab.argc(), ab.argv());
}
}    // namespace

TEST(CliArgsDeathTest, McpClientWithoutPortExits)
{
    EXPECT_EXIT(parseMcpClientWithoutPort(), ::testing::ExitedWithCode(2), ".*--mcp-client requires --port.*");
}

TEST(CliArgsDeathTest, McpClientWithHeadlessExits)
{
    EXPECT_EXIT(parseMcpClientWithHeadless(), ::testing::ExitedWithCode(2), ".*--headless and --mcp-client are mutually exclusive.*");
}

TEST(CliArgsDeathTest, McpClientWithMcpServerExits)
{
    EXPECT_EXIT(parseMcpClientWithMcpServer(), ::testing::ExitedWithCode(2), ".*--mcp-server and --mcp-client are mutually exclusive.*");
}

TEST(CliArgsDeathTest, McpClientWithMcpPortExits)
{
    EXPECT_EXIT(parseMcpClientWithMcpPort(), ::testing::ExitedWithCode(2), ".*--mcp-client and --mcp-port are mutually exclusive.*");
}

TEST(CliArgsDeathTest, McpPortWithHeadlessExits)
{
    EXPECT_EXIT(parseMcpPortWithHeadless(), ::testing::ExitedWithCode(2), ".*--headless and --mcp-port are mutually exclusive.*");
}

TEST(CliArgsDeathTest, McpPortWithMcpServerExits)
{
    EXPECT_EXIT(parseMcpPortWithMcpServer(), ::testing::ExitedWithCode(2), ".*--mcp-server and --mcp-port are mutually exclusive.*");
}

TEST(CliArgsDeathTest, InvalidMcpPortExits)
{
    EXPECT_EXIT(parseInvalidMcpPort(), ::testing::ExitedWithCode(2), ".*must be a valid integer.*");
}

TEST(CliArgsDeathTest, NegativeMcpPortExits)
{
    EXPECT_EXIT(parseNegativeMcpPort(), ::testing::ExitedWithCode(2), ".*must be a non-negative integer.*");
}

TEST(CliArgsDeathTest, InvalidPortExits)
{
    EXPECT_EXIT(parseInvalidPort(), ::testing::ExitedWithCode(2), ".*must be a valid integer.*");
}

TEST(CliArgsDeathTest, ZeroPortExits)
{
    EXPECT_EXIT(parseZeroPort(), ::testing::ExitedWithCode(2), ".*must be a positive integer.*");
}

TEST(CliArgsDeathTest, NegativePortExits)
{
    EXPECT_EXIT(parseNegativePort(), ::testing::ExitedWithCode(2), ".*must be a positive integer.*");
}
