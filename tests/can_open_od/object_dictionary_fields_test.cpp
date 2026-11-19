#include "utils/communication/can_open/object_dictionary.h"

#include <gtest/gtest.h>

using namespace Frasy::CanOpen;

// What Lua needs on top of what the GUI shows: type codes, limits, object types and the number of sub-indexes.

namespace {
constexpr std::string_view s_eds = R"(
[2000]
ParameterName=Level
ObjectType=0x7
DataType=0x0003
AccessType=rw
DefaultValue=5
LowLimit=0
HighLimit=100
PDOMapping=0

[2001]
ParameterName=Label
ObjectType=0x7
DataType=0x0009
AccessType=rw
StringLengthMin=4

[2002]
ParameterName=Raw
ObjectType=0x2
DataType=0x000F
AccessType=rw

[2003]
ParameterName=Table
ObjectType=0x8
SubNumber=0x3

[2003sub0]
ParameterName=Count
DataType=0x0005
AccessType=ro

[2003sub1]
ParameterName=First
ObjectType=0x7
DataType=0x0004
AccessType=rw

[2003sub2]
ParameterName=Second
ObjectType=0x7
DataType=0x0004
AccessType=rw

[2004]
ParameterName=Compact
ObjectType=0x8
CompactSubObj=2
DataType=0x0004
AccessType=rw
)";

constexpr std::string_view s_xdd = R"(<?xml version="1.0" encoding="utf-8"?>
<ISO15745ProfileContainer xmlns="http://www.canopen.org/xml/1.1">
  <ISO15745Profile>
    <ProfileBody xmlns:q1="http://www.canopen.org/xml/1.1">
      <q1:ApplicationProcess>
        <q1:parameterList>
          <q1:parameter uniqueID="UID_OBJ_2000" access="readWrite">
            <INT />
            <q1:defaultValue value="5" />
            <q1:allowedValues>
              <q1:range><q1:minValue value="0" /><q1:maxValue value="100" /></q1:range>
            </q1:allowedValues>
          </q1:parameter>
          <q1:parameter uniqueID="UID_OBJ_2002" access="readWrite"><DOMAIN /></q1:parameter>
        </q1:parameterList>
      </q1:ApplicationProcess>
    </ProfileBody>
    <ProfileBody xmlns:q2="http://www.canopen.org/xml/1.1">
      <ApplicationLayers>
        <q2:CANopenObjectList>
          <CANopenObject index="2000" name="Level" objectType="7" uniqueIDRef="UID_OBJ_2000" />
          <CANopenObject index="2002" name="Raw" objectType="2" uniqueIDRef="UID_OBJ_2002" />
        </q2:CANopenObjectList>
      </ApplicationLayers>
    </ProfileBody>
  </ISO15745Profile>
</ISO15745ProfileContainer>)";
}    // namespace

TEST(ObjectDictionaryFields, EdsVariable)
{
    auto od = ObjectDictionary::parseEds(s_eds);
    ASSERT_TRUE(od.has_value()) << od.error();
    const auto& entry = od->findObject(0x2000)->entries.at(0);
    EXPECT_EQ(entry.dataType, 3);
    EXPECT_EQ(entry.objectType, OdObjectType::Var);
    EXPECT_EQ(entry.lowLimit, "0");
    EXPECT_EQ(entry.highLimit, "100");
    EXPECT_FALSE(entry.stringLengthMin.has_value());

    EXPECT_EQ(od->findObject(0x2001)->entries.at(0).stringLengthMin, 4u);
}

TEST(ObjectDictionaryFields, ObjectTypeIsKept)
{
    auto od = ObjectDictionary::parseEds(s_eds);
    ASSERT_TRUE(od.has_value()) << od.error();
    EXPECT_EQ(od->findObject(0x2002)->type, OdObjectType::Domain);
    EXPECT_EQ(od->findObject(0x2002)->entries.at(0).objectType, OdObjectType::Domain);
    EXPECT_EQ(od->findObject(0x2002)->entries.at(0).dataType, 0x0F);
}

TEST(ObjectDictionaryFields, SubNumber)
{
    auto od = ObjectDictionary::parseEds(s_eds);
    ASSERT_TRUE(od.has_value()) << od.error();
    const auto* table = od->findObject(0x2003);
    ASSERT_NE(table, nullptr);
    EXPECT_EQ(table->subNumber, 3u);
    // A sub-index without an ObjectType is a variable.
    EXPECT_EQ(table->entries.at(0).objectType, OdObjectType::Var);
    EXPECT_EQ(table->entries.at(1).objectType, OdObjectType::Var);
}

TEST(ObjectDictionaryFields, CompactSubNumberCountsTheLengthEntry)
{
    auto od = ObjectDictionary::parseEds(s_eds);
    ASSERT_TRUE(od.has_value()) << od.error();
    const auto* compact = od->findObject(0x2004);
    ASSERT_NE(compact, nullptr);
    EXPECT_EQ(compact->subNumber, 3u);
    for (const auto& entry : compact->entries) {
        EXPECT_EQ(entry.objectType, OdObjectType::Var);
    }
}

TEST(ObjectDictionaryFields, XddLimitsAndTypes)
{
    auto od = ObjectDictionary::parseXdd(s_xdd);
    ASSERT_TRUE(od.has_value()) << od.error();
    const auto& level = od->findObject(0x2000)->entries.at(0);
    EXPECT_EQ(level.dataType, 3);
    EXPECT_EQ(level.lowLimit, "0");
    EXPECT_EQ(level.highLimit, "100");
    EXPECT_EQ(level.defaultValue, "5");

    EXPECT_EQ(od->findObject(0x2002)->type, OdObjectType::Domain);
    EXPECT_EQ(od->findObject(0x2002)->entries.at(0).dataType, 0x0F);
}

TEST(ObjectDictionaryFields, ParseDetectsTheFormat)
{
    EXPECT_TRUE(ObjectDictionary::parse(s_eds).has_value());
    EXPECT_TRUE(ObjectDictionary::parse(s_xdd).has_value());
    EXPECT_TRUE(ObjectDictionary::parse(std::string {"\xEF\xBB\xBF"} + std::string {s_xdd}).has_value());
    EXPECT_FALSE(ObjectDictionary::parse("nothing to see").has_value());
}
