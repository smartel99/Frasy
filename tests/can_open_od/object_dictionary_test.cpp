#include "utils/communication/can_open/object_dictionary.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <format>
#include <fstream>

using namespace Frasy::CanOpen;

namespace {
constexpr std::string_view s_eds = R"(
[FileInfo]
FileName=test.eds

[DeviceInfo]
VendorName=ACME
ProductName=Widget

[1000]
ParameterName=Device type
ObjectType=0x7
DataType=0x0007
AccessType=ro
DefaultValue=0x00000001
PDOMapping=0

[1018]
ParameterName=Identity
ObjectType=0x9
SubNumber=0x3

[1018sub0]
ParameterName=Highest sub-index supported
ObjectType=0x7
DataType=0x0005
AccessType=ro

[1018sub2]
ParameterName=Product code
ObjectType=0x7
DataType=0x0007
AccessType=rw
PDOMapping=1

[1018sub1]
ParameterName=Vendor-ID
ObjectType=0x7
DataType=0x0007
AccessType=const

[2000]
ParameterName=Name
DataType=0x0009
AccessType=wo

[2001]
ParameterName=Samples
ObjectType=0x8
CompactSubObj=3
DataType=0x0004
AccessType=rw
)";

constexpr std::string_view s_xdd = R"(<?xml version="1.0" encoding="utf-8"?>
<ISO15745ProfileContainer xmlns="http://www.canopen.org/xml/1.1">
  <ISO15745Profile>
    <ProfileBody xmlns:q1="http://www.canopen.org/xml/1.1">
      <q1:DeviceIdentity>
        <q1:vendorName>ACME &amp; Co</q1:vendorName>
        <q1:productName>Widget</q1:productName>
      </q1:DeviceIdentity>
      <q1:ApplicationProcess>
        <q1:dataTypeList>
          <q1:array name="Array" uniqueID="UID_ARR_2001">
            <q1:subrange lowerLimit="0" upperLimit="2" />
            <DINT />
          </q1:array>
          <q1:struct name="Identity" uniqueID="UID_REC_1018">
            <q1:varDeclaration name="Highest sub-index" uniqueID="UID_RECSUB_101800"><USINT /></q1:varDeclaration>
            <q1:varDeclaration name="Vendor-ID" uniqueID="UID_RECSUB_101801"><UDINT /></q1:varDeclaration>
          </q1:struct>
        </q1:dataTypeList>
        <q1:parameterList>
          <q1:parameter uniqueID="UID_OBJ_1000">
            <description lang="en">Device type
description</description>
            <UDINT />
            <q1:defaultValue value="0x00000001" />
          </q1:parameter>
          <q1:parameter uniqueID="UID_OBJ_1018">
            <q1:dataTypeIDRef uniqueIDRef="UID_REC_1018" />
          </q1:parameter>
          <q1:parameter uniqueID="UID_SUB_101800"><label lang="en">Highest sub-index</label><description lang="en">  Number of entries </description><USINT /></q1:parameter>
          <q1:parameter uniqueID="UID_SUB_101801" access="readWrite"><label lang="en">Vendor-ID</label></q1:parameter>
          <q1:parameter uniqueID="UID_OBJ_2000" access="write"><STRING /></q1:parameter>
          <q1:parameter uniqueID="UID_OBJ_2001"><q1:dataTypeIDRef uniqueIDRef="UID_ARR_2001" /></q1:parameter>
          <q1:parameter uniqueID="UID_SUB_200100" access="readWrite"><USINT /></q1:parameter>
          <q1:parameter uniqueID="UID_SUB_200101" access="readWrite" />
        </q1:parameterList>
      </q1:ApplicationProcess>
    </ProfileBody>
    <ProfileBody xmlns:q2="http://www.canopen.org/xml/1.1">
      <ApplicationLayers>
        <q2:CANopenObjectList>
          <CANopenObject index="1000" name="Device type" objectType="7" PDOmapping="no" uniqueIDRef="UID_OBJ_1000" />
          <CANopenObject index="1018" name="Identity" objectType="9" uniqueIDRef="UID_OBJ_1018" subNumber="2">
            <CANopenSubObject subIndex="01" name="Vendor-ID" objectType="7" PDOmapping="TPDO" uniqueIDRef="UID_SUB_101801" />
            <CANopenSubObject subIndex="00" name="Highest sub-index" objectType="7" PDOmapping="no" uniqueIDRef="UID_SUB_101800" />
          </CANopenObject>
          <CANopenObject index="2000" name="Name" objectType="7" uniqueIDRef="UID_OBJ_2000" />
          <CANopenObject index="2001" name="Samples" objectType="8" uniqueIDRef="UID_OBJ_2001" subNumber="2">
            <CANopenSubObject subIndex="00" name="Count" objectType="7" uniqueIDRef="UID_SUB_200100" />
            <CANopenSubObject subIndex="01" name="Sample" objectType="7" uniqueIDRef="UID_SUB_200101" />
          </CANopenObject>
        </q2:CANopenObjectList>
      </ApplicationLayers>
    </ProfileBody>
  </ISO15745Profile>
</ISO15745ProfileContainer>)";

std::filesystem::path canOpenDir()
{
    return std::filesystem::path {FRASY_TEST_CAN_OPEN_DIR};
}
}    // namespace

TEST(ObjectDictionaryEds, ParsesDeviceInfo)
{
    auto od = ObjectDictionary::parseEds(s_eds);
    ASSERT_TRUE(od.has_value()) << od.error();
    EXPECT_EQ(od->getVendorName(), "ACME");
    EXPECT_EQ(od->getProductName(), "Widget");
}

TEST(ObjectDictionaryEds, ParsesVariable)
{
    auto od = ObjectDictionary::parseEds(s_eds);
    ASSERT_TRUE(od.has_value()) << od.error();

    const auto* object = od->findObject(0x1000);
    ASSERT_NE(object, nullptr);
    EXPECT_EQ(object->name, "Device type");
    EXPECT_EQ(object->type, OdObjectType::Var);
    EXPECT_FALSE(object->hasSubIndexes());
    ASSERT_EQ(object->entries.size(), 1u);
    EXPECT_EQ(object->entries[0].subIndex, 0);
    EXPECT_EQ(object->entries[0].varType, VarType::Unsigned32);
    EXPECT_EQ(object->entries[0].dataTypeName, "UNSIGNED32");
    EXPECT_EQ(object->entries[0].access, OdAccess::ReadOnly);
    EXPECT_EQ(object->entries[0].defaultValue, "0x00000001");
    EXPECT_FALSE(object->entries[0].isPdoMappable);
}

TEST(ObjectDictionaryEds, ParsesRecordSortedBySubIndex)
{
    auto od = ObjectDictionary::parseEds(s_eds);
    ASSERT_TRUE(od.has_value()) << od.error();

    const auto* object = od->findObject(0x1018);
    ASSERT_NE(object, nullptr);
    EXPECT_TRUE(object->hasSubIndexes());
    ASSERT_EQ(object->entries.size(), 3u);
    EXPECT_EQ(object->entries[0].subIndex, 0);
    EXPECT_EQ(object->entries[1].subIndex, 1);
    EXPECT_EQ(object->entries[1].access, OdAccess::Const);
    EXPECT_EQ(object->entries[2].subIndex, 2);
    EXPECT_EQ(object->entries[2].access, OdAccess::ReadWrite);
    EXPECT_TRUE(object->entries[2].isPdoMappable);
}

TEST(ObjectDictionaryEds, ParsesStringAndWriteOnly)
{
    auto od = ObjectDictionary::parseEds(s_eds);
    ASSERT_TRUE(od.has_value()) << od.error();
    const auto* object = od->findObject(0x2000);
    ASSERT_NE(object, nullptr);
    ASSERT_EQ(object->entries.size(), 1u);
    EXPECT_EQ(object->entries[0].varType, VarType::String);
    EXPECT_EQ(object->entries[0].access, OdAccess::WriteOnly);
}

TEST(ObjectDictionaryEds, ExpandsCompactSubObjects)
{
    auto od = ObjectDictionary::parseEds(s_eds);
    ASSERT_TRUE(od.has_value()) << od.error();
    const auto* object = od->findObject(0x2001);
    ASSERT_NE(object, nullptr);
    ASSERT_EQ(object->entries.size(), 4u);
    EXPECT_EQ(object->entries[0].varType, VarType::Unsigned8);
    EXPECT_EQ(object->entries[3].subIndex, 3);
    EXPECT_EQ(object->entries[3].varType, VarType::Signed32);
    EXPECT_EQ(object->entries[3].access, OdAccess::ReadWrite);
}

TEST(ObjectDictionaryEds, ObjectsAreSorted)
{
    auto od = ObjectDictionary::parseEds(s_eds);
    ASSERT_TRUE(od.has_value()) << od.error();
    ASSERT_EQ(od->getObjects().size(), 4u);
    EXPECT_EQ(od->getObjects()[0].index, 0x1000);
    EXPECT_EQ(od->getObjects()[3].index, 0x2001);
}

TEST(ObjectDictionaryEds, RejectsNonEds)
{
    EXPECT_FALSE(ObjectDictionary::parseEds("hello world").has_value());
    EXPECT_FALSE(ObjectDictionary::parseEds("").has_value());
}

TEST(ObjectDictionaryXdd, ParsesDeviceInfo)
{
    auto od = ObjectDictionary::parseXdd(s_xdd);
    ASSERT_TRUE(od.has_value()) << od.error();
    EXPECT_EQ(od->getVendorName(), "ACME & Co");
    EXPECT_EQ(od->getProductName(), "Widget");
}

TEST(ObjectDictionaryXdd, ParsesVariable)
{
    auto od = ObjectDictionary::parseXdd(s_xdd);
    ASSERT_TRUE(od.has_value()) << od.error();
    const auto* object = od->findObject(0x1000);
    ASSERT_NE(object, nullptr);
    EXPECT_EQ(object->description, "Device type\ndescription");
    ASSERT_EQ(object->entries.size(), 1u);
    EXPECT_EQ(object->entries[0].description, "Device type\ndescription");
    EXPECT_EQ(object->entries[0].varType, VarType::Unsigned32);
    EXPECT_EQ(object->entries[0].access, OdAccess::ReadOnly);    // Default of the access attribute.
    EXPECT_EQ(object->entries[0].defaultValue, "0x00000001");

    const auto* name = od->findObject(0x2000);
    ASSERT_NE(name, nullptr);
    EXPECT_EQ(name->entries[0].varType, VarType::String);
    EXPECT_EQ(name->entries[0].access, OdAccess::WriteOnly);
}

TEST(ObjectDictionaryXdd, ParsesRecordWithTypesFromStructure)
{
    auto od = ObjectDictionary::parseXdd(s_xdd);
    ASSERT_TRUE(od.has_value()) << od.error();
    const auto* object = od->findObject(0x1018);
    ASSERT_NE(object, nullptr);
    EXPECT_TRUE(object->hasSubIndexes());
    ASSERT_EQ(object->entries.size(), 2u);
    EXPECT_EQ(object->entries[0].subIndex, 0);
    EXPECT_EQ(object->entries[0].varType, VarType::Unsigned8);
    EXPECT_EQ(object->entries[0].description, "Number of entries");    // Trimmed.
    EXPECT_TRUE(object->entries[1].description.empty());
    EXPECT_EQ(object->entries[1].subIndex, 1);
    // The parameter has no type of its own, it comes from the member of the structure.
    EXPECT_EQ(object->entries[1].varType, VarType::Unsigned32);
    EXPECT_EQ(object->entries[1].access, OdAccess::ReadWrite);
    EXPECT_TRUE(object->entries[1].isPdoMappable);
}

TEST(ObjectDictionaryXdd, ParsesArrayWithTypeFromElement)
{
    auto od = ObjectDictionary::parseXdd(s_xdd);
    ASSERT_TRUE(od.has_value()) << od.error();
    const auto* object = od->findObject(0x2001);
    ASSERT_NE(object, nullptr);
    ASSERT_EQ(object->entries.size(), 2u);
    EXPECT_EQ(object->entries[0].varType, VarType::Unsigned8);
    EXPECT_EQ(object->entries[1].varType, VarType::Signed32);
}

TEST(ObjectDictionaryXdd, RejectsMalformedXml)
{
    EXPECT_FALSE(ObjectDictionary::parseXdd("<a><b></a>").has_value());
    EXPECT_FALSE(ObjectDictionary::parseXdd("<a>").has_value());
    EXPECT_FALSE(ObjectDictionary::parseXdd("not xml").has_value());
    EXPECT_FALSE(ObjectDictionary::parseXdd("<a/>").has_value());    // Valid XML, but no object list.
}

TEST(ObjectDictionaryXdd, RejectsExcessiveNesting)
{
    std::string deep;
    for (int i = 0; i < 1000; ++i) {
        deep += "<a>";
    }
    EXPECT_FALSE(ObjectDictionary::parseXdd(deep).has_value());
}

TEST(ObjectDictionaryLoad, DetectsFormatFromContent)
{
    const auto path = std::filesystem::temp_directory_path() / "frasy_test_od.dat";
    {
        std::ofstream(path, std::ios::binary) << s_eds;
    }
    auto edsOd = ObjectDictionary::load(path);
    ASSERT_TRUE(edsOd.has_value()) << edsOd.error();
    EXPECT_EQ(edsOd->getObjects().size(), 4u);
    {
        std::ofstream(path, std::ios::binary) << s_xdd;
    }
    auto xddOd = ObjectDictionary::load(path);
    ASSERT_TRUE(xddOd.has_value()) << xddOd.error();
    EXPECT_EQ(xddOd->getObjects().size(), 4u);
    std::filesystem::remove(path);
}

TEST(ObjectDictionaryLoad, MissingFileIsAnError)
{
    EXPECT_FALSE(ObjectDictionary::load("this/file/does/not/exist.eds").has_value());
}

TEST(ObjectDictionaryLoad, BoardXddHasDeviceType)
{
    auto od = ObjectDictionary::load("lua/core/cep/eds/daq.xdd");
    ASSERT_TRUE(od.has_value()) << od.error();
    const auto* object = od->findObject(0x1000);
    ASSERT_NE(object, nullptr);
    ASSERT_EQ(object->entries.size(), 1u);
    EXPECT_EQ(object->entries[0].varType, VarType::Unsigned32);
    EXPECT_TRUE(isReadable(object->entries[0].access));
}

// The XDD and the EDS of the Frasy node are generated from the same dictionary, so both parsers must agree. The XDD
// also describes the SDO client parameters (0x1280), which the EDS does not.
TEST(ObjectDictionaryLoad, FrasyEdsAndXddAgree)
{
    auto eds = ObjectDictionary::load(canOpenDir() / "frasy.eds");
    auto xdd = ObjectDictionary::load(canOpenDir() / "frasy.xdd");
    ASSERT_TRUE(eds.has_value()) << eds.error();
    ASSERT_TRUE(xdd.has_value()) << xdd.error();
    ASSERT_GE(xdd->getObjects().size(), eds->getObjects().size());

    for (const auto& e : eds->getObjects()) {
        const auto* x = xdd->findObject(e.index);
        ASSERT_NE(x, nullptr) << std::format("0x{:04X}", e.index);
        EXPECT_EQ(e.type, x->type) << std::format("0x{:04X}", e.index);
        ASSERT_EQ(e.entries.size(), x->entries.size()) << std::format("0x{:04X}", e.index);
        for (std::size_t i = 0; i < e.entries.size(); ++i) {
            const auto what = std::format("0x{:04X} sub {}", e.index, e.entries[i].subIndex);
            EXPECT_EQ(e.entries[i].subIndex, x->entries[i].subIndex) << what;
            EXPECT_EQ(e.entries[i].varType, x->entries[i].varType) << what;
            EXPECT_EQ(e.entries[i].access, x->entries[i].access) << what;
        }
    }
}
