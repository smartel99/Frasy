#include "utils/communication/can_open/od_value.h"
#include "utils/communication/can_open/object_dictionary.h"
#include "utils/communication/can_open/types.h"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <cstring>

using namespace Frasy::CanOpen;

namespace {
template<typename T>
std::vector<uint8_t> bytesOf(T value)
{
    std::vector<uint8_t> out(sizeof(T));
    std::memcpy(out.data(), &value, sizeof(T));
    return out;
}
}    // namespace

TEST(OdValueFormat, UnsignedShowsHex)
{
    EXPECT_EQ(OdValue::format(VarType::Unsigned32, bytesOf<uint32_t>(0x1234)), "4660 (0x00001234)");
    EXPECT_EQ(OdValue::format(VarType::Unsigned8, bytesOf<uint8_t>(5)), "5 (0x05)");
    EXPECT_EQ(OdValue::format(VarType::Unsigned16, bytesOf<uint16_t>(5), false), "5");
}

TEST(OdValueFormat, SignedAndReal)
{
    EXPECT_EQ(OdValue::format(VarType::Signed16, bytesOf<int16_t>(-2)), "-2");
    EXPECT_EQ(OdValue::format(VarType::Real32, bytesOf<float>(1.5f)), "1.5");
    EXPECT_EQ(OdValue::format(VarType::Real64, bytesOf<double>(-0.25)), "-0.25");
    EXPECT_EQ(OdValue::format(VarType::Boolean, bytesOf<uint8_t>(1)), "true");
    EXPECT_EQ(OdValue::format(VarType::Boolean, bytesOf<uint8_t>(0)), "false");
}

TEST(OdValueFormat, StringStopsAtNull)
{
    const std::array<uint8_t, 6> bytes = {'a', 'b', 'c', 0, 'x', 'y'};
    EXPECT_EQ(OdValue::format(VarType::String, bytes), "abc");
}

TEST(OdValueFormat, RawAndWrongSizeAreShownAsHex)
{
    const std::array<uint8_t, 3> bytes = {0x01, 0xAB, 0xFF};
    EXPECT_EQ(OdValue::format(VarType::Undefined, bytes), "01 AB FF");
    EXPECT_NE(OdValue::format(VarType::Unsigned32, bytes).find("01 AB FF"), std::string::npos);
    EXPECT_NE(OdValue::format(VarType::Unsigned32, bytes).find("unexpected size"), std::string::npos);
}

TEST(OdValueParse, Integers)
{
    EXPECT_EQ(OdValue::parse(VarType::Unsigned16, "1234").value(), bytesOf<uint16_t>(1234));
    EXPECT_EQ(OdValue::parse(VarType::Unsigned16, " 0x1234 ").value(), bytesOf<uint16_t>(0x1234));
    EXPECT_EQ(OdValue::parse(VarType::Signed32, "-5").value(), bytesOf<int32_t>(-5));
    EXPECT_EQ(OdValue::parse(VarType::Signed8, "0xFF").value(), bytesOf<int8_t>(-1));
    EXPECT_EQ(OdValue::parse(VarType::Unsigned64, "18446744073709551615").value(), bytesOf<uint64_t>(UINT64_MAX));
}

TEST(OdValueParse, IntegerErrors)
{
    EXPECT_FALSE(OdValue::parse(VarType::Unsigned8, "256").has_value());
    EXPECT_FALSE(OdValue::parse(VarType::Unsigned8, "-1").has_value());
    EXPECT_FALSE(OdValue::parse(VarType::Signed8, "128").has_value());
    EXPECT_FALSE(OdValue::parse(VarType::Unsigned8, "0x100").has_value());
    EXPECT_FALSE(OdValue::parse(VarType::Unsigned8, "12abc").has_value());
    EXPECT_FALSE(OdValue::parse(VarType::Unsigned8, "0x").has_value());
    EXPECT_FALSE(OdValue::parse(VarType::Unsigned8, "").has_value());
}

TEST(OdValueParse, RealAndBoolean)
{
    EXPECT_EQ(OdValue::parse(VarType::Real32, "2.5").value(), bytesOf<float>(2.5f));
    EXPECT_EQ(OdValue::parse(VarType::Real64, "-1e3").value(), bytesOf<double>(-1000.0));
    EXPECT_FALSE(OdValue::parse(VarType::Real32, "abc").has_value());
    EXPECT_EQ(OdValue::parse(VarType::Boolean, "TRUE").value(), std::vector<uint8_t> {1});
    EXPECT_EQ(OdValue::parse(VarType::Boolean, "0").value(), std::vector<uint8_t> {0});
    EXPECT_FALSE(OdValue::parse(VarType::Boolean, "maybe").has_value());
}

TEST(OdValueParse, StringIsNullTerminated)
{
    EXPECT_EQ(OdValue::parse(VarType::String, "hi").value(), (std::vector<uint8_t> {'h', 'i', 0}));
}

TEST(OdValueParse, RawHex)
{
    const std::vector<uint8_t> expected = {0x01, 0xAB, 0xFF};
    EXPECT_EQ(OdValue::parse(VarType::Undefined, "01 AB FF").value(), expected);
    EXPECT_EQ(OdValue::parse(VarType::Undefined, "01ABFF").value(), expected);
    EXPECT_EQ(OdValue::parse(VarType::Undefined, "0x01, 0xab, 0xff").value(), expected);
    EXPECT_EQ(OdValue::parse(VarType::Undefined, "1 a").value(), (std::vector<uint8_t> {0x01, 0x0A}));
    EXPECT_FALSE(OdValue::parse(VarType::Undefined, "01A").has_value());
    EXPECT_FALSE(OdValue::parse(VarType::Undefined, "zz").has_value());
    EXPECT_FALSE(OdValue::parse(VarType::Undefined, "  ").has_value());
}

TEST(OdValueParse, OctetStringDecimalAndHex)
{
    const std::vector<uint8_t> expected = {0x01, 0xAB, 0xFF, 0x00};
    EXPECT_EQ(OdValue::parseOctetString("1 171 255 0").value(), expected);
    EXPECT_EQ(OdValue::parseOctetString("0x01,0xab,0xFF,0x0").value(), expected);
    EXPECT_EQ(OdValue::parseOctetString("0x01\t171, 0XFF ,  0").value(), expected);
    EXPECT_TRUE(OdValue::parseOctetString("").value().empty());
    EXPECT_TRUE(OdValue::parseOctetString(" , ").value().empty());
}

TEST(OdValueParse, OctetStringErrors)
{
    EXPECT_FALSE(OdValue::parseOctetString("256").has_value());
    EXPECT_FALSE(OdValue::parseOctetString("0x100").has_value());
    EXPECT_FALSE(OdValue::parseOctetString("99999999999999999999").has_value());
    EXPECT_FALSE(OdValue::parseOctetString("-1").has_value());
    EXPECT_FALSE(OdValue::parseOctetString("+1").has_value());
    EXPECT_FALSE(OdValue::parseOctetString("0x-1").has_value());
    EXPECT_FALSE(OdValue::parseOctetString("AB").has_value());
    EXPECT_FALSE(OdValue::parseOctetString("0x").has_value());
    EXPECT_FALSE(OdValue::parseOctetString("1;2").has_value());
}

TEST(OdValueFormat, OctetStringRoundTrip)
{
    const std::vector<uint8_t> bytes = {0x01, 0xAB, 0xFF};
    EXPECT_EQ(OdValue::formatOctetString(bytes), "0x01 0xAB 0xFF");
    EXPECT_EQ(OdValue::parseOctetString(OdValue::formatOctetString(bytes)).value(), bytes);
}

TEST(OdValueParse, EntryStringsArePaddedToStringLengthMin)
{
    OdEntry octets;
    octets.dataType        = static_cast<uint16_t>(DataType::octetString);
    octets.stringLengthMin = 6;
    EXPECT_EQ(OdValue::parse(octets, "0x01 2").value(), (std::vector<uint8_t> {1, 2, 0, 0, 0, 0}));
    EXPECT_EQ(OdValue::parse(octets, "").value(), std::vector<uint8_t>(6, 0));
    EXPECT_FALSE(OdValue::parse(octets, "1 2 3 4 5 6 7").has_value());

    OdEntry text;
    text.dataType        = static_cast<uint16_t>(DataType::visibleString);
    text.varType         = VarType::String;
    text.stringLengthMin = 4;
    EXPECT_EQ(OdValue::parse(text, "hi").value(), (std::vector<uint8_t> {'h', 'i', 0, 0}));
    EXPECT_EQ(OdValue::parse(text, "abcd").value(), (std::vector<uint8_t> {'a', 'b', 'c', 'd'}));
    EXPECT_FALSE(OdValue::parse(text, "abcde").has_value());
}

TEST(OdValueParse, EntryStringsWithoutStringLengthMinOnlyAcceptEmpty)
{
    OdEntry octets;
    octets.dataType = static_cast<uint16_t>(DataType::octetString);
    EXPECT_TRUE(OdValue::parse(octets, "").value().empty());
    EXPECT_FALSE(OdValue::parse(octets, "1").has_value());

    OdEntry text;
    text.dataType = static_cast<uint16_t>(DataType::visibleString);
    EXPECT_TRUE(OdValue::parse(text, "").value().empty());
    EXPECT_FALSE(OdValue::parse(text, "a").has_value());
}

TEST(OdValueParse, EntryOtherTypesUseVarType)
{
    OdEntry entry;
    entry.dataType = static_cast<uint16_t>(DataType::unsigned16);
    entry.varType  = VarType::Unsigned16;
    EXPECT_EQ(OdValue::parse(entry, "0x1234").value(), bytesOf<uint16_t>(0x1234));
}

TEST(OdValueRoundTrip, ParseThenFormat)
{
    for (auto text : {"0", "42", "255"}) {
        auto bytes = OdValue::parse(VarType::Unsigned8, text);
        ASSERT_TRUE(bytes.has_value());
        EXPECT_EQ(OdValue::format(VarType::Unsigned8, *bytes, false), text);
    }
}
