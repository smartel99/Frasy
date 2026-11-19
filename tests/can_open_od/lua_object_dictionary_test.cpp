#include "lua_test_fixture.h"

#include "utils/communication/can_open/object_dictionary_registry.h"
#include "utils/lua/object_dictionary_binding.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

namespace {
// The same dictionary, described as an EDS and as a XDD.
constexpr const char* s_eds = R"(
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
DefaultValue=0x02
PDOMapping=0

[1018sub1]
ParameterName=Vendor-ID
ObjectType=0x7
DataType=0x0007
AccessType=rw
DefaultValue=0x00000069
PDOMapping=1

[1018sub2]
ParameterName=Product code
ObjectType=0x7
DataType=0x0007
AccessType=ro
PDOMapping=0

[2000]
ParameterName=Name
ObjectType=0x7
DataType=0x0009
AccessType=wo
PDOMapping=0
)";

constexpr const char* s_xdd = R"(<?xml version="1.0" encoding="utf-8"?>
<ISO15745ProfileContainer xmlns="http://www.canopen.org/xml/1.1">
  <ISO15745Profile>
    <ProfileBody xmlns:q1="http://www.canopen.org/xml/1.1">
      <q1:ApplicationProcess>
        <q1:dataTypeList>
          <q1:struct name="Identity" uniqueID="UID_REC_1018">
            <q1:varDeclaration name="Highest sub-index supported" uniqueID="UID_RECSUB_101800"><USINT /></q1:varDeclaration>
            <q1:varDeclaration name="Vendor-ID" uniqueID="UID_RECSUB_101801"><UDINT /></q1:varDeclaration>
            <q1:varDeclaration name="Product code" uniqueID="UID_RECSUB_101802"><UDINT /></q1:varDeclaration>
          </q1:struct>
        </q1:dataTypeList>
        <q1:parameterList>
          <q1:parameter uniqueID="UID_OBJ_1000">
            <UDINT />
            <q1:defaultValue value="0x00000001" />
          </q1:parameter>
          <q1:parameter uniqueID="UID_OBJ_1018">
            <q1:dataTypeIDRef uniqueIDRef="UID_REC_1018" />
          </q1:parameter>
          <q1:parameter uniqueID="UID_SUB_101800">
            <USINT />
            <q1:defaultValue value="0x02" />
          </q1:parameter>
          <q1:parameter uniqueID="UID_SUB_101801" access="readWrite">
            <q1:defaultValue value="0x00000069" />
          </q1:parameter>
          <q1:parameter uniqueID="UID_SUB_101802" />
          <q1:parameter uniqueID="UID_OBJ_2000" access="write"><STRING /></q1:parameter>
        </q1:parameterList>
      </q1:ApplicationProcess>
    </ProfileBody>
    <ProfileBody xmlns:q2="http://www.canopen.org/xml/1.1">
      <ApplicationLayers>
        <q2:CANopenObjectList>
          <CANopenObject index="1000" name="Device type" objectType="7" PDOmapping="no" uniqueIDRef="UID_OBJ_1000" />
          <CANopenObject index="1018" name="Identity" objectType="9" uniqueIDRef="UID_OBJ_1018" subNumber="3">
            <CANopenSubObject subIndex="00" name="Highest sub-index supported" objectType="7" PDOmapping="no" uniqueIDRef="UID_SUB_101800" />
            <CANopenSubObject subIndex="01" name="Vendor-ID" objectType="7" PDOmapping="TPDO" uniqueIDRef="UID_SUB_101801" />
            <CANopenSubObject subIndex="02" name="Product code" objectType="7" PDOmapping="no" uniqueIDRef="UID_SUB_101802" />
          </CANopenObject>
          <CANopenObject index="2000" name="Name" objectType="7" PDOmapping="no" uniqueIDRef="UID_OBJ_2000" />
        </q2:CANopenObjectList>
      </ApplicationLayers>
    </ProfileBody>
  </ISO15745Profile>
</ISO15745ProfileContainer>)";
}    // namespace

class LuaObjectDictionaryTest : public LuaTestFixture {
protected:
    void registerMocks() override
    {
        LuaTestFixture::registerMocks();
        // The dictionaries are parsed by C++, Lua builds its table from the result.
        Frasy::Lua::registerObjectDictionary(lua);
    }

    void SetUp() override
    {
        LuaTestFixture::SetUp();
        lua.script("OdParser = require('lua.core.can_open.object_dictionary')");
        Frasy::CanOpen::ObjectDictionaryRegistry::get().clear();
        lua["EdsText"] = std::string {s_eds};
        lua["XddText"] = std::string {s_xdd};
    }

    //! Writes a file in the temporary directory, to be loaded with LoadFile. Removed with the fixture.
    std::string writeTempFile(const std::string& name, const std::string& content)
    {
        auto path = std::filesystem::temp_directory_path() / name;
        std::ofstream(path, std::ios::binary) << content;
        m_tempFiles.push_back(path);
        return path.generic_string();
    }

    void TearDown() override
    {
        for (const auto& path : m_tempFiles) {
            std::filesystem::remove(path);
        }
    }

private:
    std::vector<std::filesystem::path> m_tempFiles;
};

TEST_F(LuaObjectDictionaryTest, Xdd_Variable)
{
    lua.script("od = OdParser.Parse(XddText)");
    EXPECT_EQ(lua.script("return od['Device type'].index").get<std::string>(), "0x1000");
    EXPECT_EQ(lua.script("return od['Device type'].subIndex").get<std::string>(), "0x0");
    EXPECT_EQ(lua.script("return od['Device type'].objectType").get<int>(), 7);
    EXPECT_EQ(lua.script("return od['Device type'].dataType").get<int>(), 7);
    EXPECT_EQ(lua.script("return od['Device type'].accessType").get<std::string>(), "ro");
    EXPECT_EQ(lua.script("return od['Device type'].defaultValue").get<int>(), 1);
    EXPECT_EQ(lua.script("return od['Device type'].value").get<int>(), 1);
}

TEST_F(LuaObjectDictionaryTest, Xdd_RecordTakesTheTypesOfTheStructure)
{
    lua.script("od = OdParser.Parse(XddText)");
    EXPECT_EQ(lua.script("return od['Identity'].objectType").get<int>(), 9);
    EXPECT_EQ(lua.script("return od['Identity'].index").get<std::string>(), "0x1018");
    EXPECT_EQ(lua.script("return od['Identity']['Highest sub-index supported'].dataType").get<int>(), 5);
    // These parameters do not declare a type, it comes from the member of the structure.
    EXPECT_EQ(lua.script("return od['Identity']['Vendor-ID'].dataType").get<int>(), 7);
    EXPECT_EQ(lua.script("return od['Identity']['Vendor-ID'].accessType").get<std::string>(), "rw");
    EXPECT_EQ(lua.script("return od['Identity']['Vendor-ID'].subIndex").get<std::string>(), "1");
    EXPECT_EQ(lua.script("return od['Identity']['Vendor-ID'].defaultValue").get<int>(), 0x69);
    EXPECT_EQ(lua.script("return od['Identity']['Product code'].dataType").get<int>(), 7);
    EXPECT_EQ(lua.script("return od['Identity'].__fields[2]").get<std::string>(), "Product code");
}

TEST_F(LuaObjectDictionaryTest, Xdd_WriteOnlyString)
{
    lua.script("od = OdParser.Parse(XddText)");
    EXPECT_EQ(lua.script("return od['Name'].dataType").get<int>(), 9);
    EXPECT_EQ(lua.script("return od['Name'].accessType").get<std::string>(), "wo");
}

TEST_F(LuaObjectDictionaryTest, EdsAndXddGiveTheSameDictionary)
{
    lua.script(R"(
        edsOd = OdParser.Parse(EdsText)
        xddOd = OdParser.Parse(XddText)
        function Compare(a, b, path)
            for k, v in pairs(a) do
                if type(v) == "table" then
                    local r = Compare(v, b[k] or {}, path .. "." .. tostring(k))
                    if r then return r end
                elseif v ~= b[k] then
                    return path .. "." .. tostring(k) .. ": " .. tostring(v) .. " ~= " .. tostring(b[k])
                end
            end
            return nil
        end
        difference = Compare(edsOd, xddOd, "od") or Compare(xddOd, edsOd, "od")
    )");
    EXPECT_TRUE(lua["difference"].get<sol::object>() == sol::nil) << lua["difference"].get<std::string>();
}

TEST_F(LuaObjectDictionaryTest, LoadFile_DetectsTheFormatFromTheContent)
{
    // The extension does not matter.
    lua["EdsFile"] = writeTempFile("frasy_od_test_eds.xdd", s_eds);
    lua["XddFile"] = writeTempFile("frasy_od_test_xdd.eds", s_xdd);
    EXPECT_EQ(lua.script("return OdParser.LoadFile(EdsFile)['Device type'].dataType").get<int>(), 7);
    EXPECT_EQ(lua.script("return OdParser.LoadFile(XddFile)['Device type'].dataType").get<int>(), 7);
}

TEST_F(LuaObjectDictionaryTest, LoadFile_AcceptsAByteOrderMark)
{
    lua["XddFile"] = writeTempFile("frasy_od_test_bom.xdd", std::string {"\xEF\xBB\xBF"} + s_xdd);
    EXPECT_EQ(lua.script("return OdParser.LoadFile(XddFile)['Device type'].dataType").get<int>(), 7);
}

TEST_F(LuaObjectDictionaryTest, LoadFile_BoardEds)
{
    EXPECT_EQ(lua.script("return OdParser.LoadFile('lua/core/cep/eds/pio.eds')['Device type'].index")
                .get<std::string>(),
              "0x1000");
}

TEST_F(LuaObjectDictionaryTest, LoadFile_UsesTheResultOfTheRegistry)
{
    // What Lua loads is shared with the GUI: the file is not parsed again.
    auto& registry = Frasy::CanOpen::ObjectDictionaryRegistry::get();
    auto  before   = registry.load("lua/core/cep/eds/pio.eds");
    ASSERT_TRUE(before.has_value()) << before.error();
    lua.script("OdParser.LoadFile('lua/core/cep/eds/pio.eds')");
    lua.script("OdParser.LoadFile('lua/core/cep/eds/pio.eds')");
    EXPECT_EQ(registry.load("lua/core/cep/eds/pio.eds")->get(), before->get());
}

TEST_F(LuaObjectDictionaryTest, LoadFile_MissingFileIsAnError)
{
    EXPECT_FALSE(lua.script("return pcall(OdParser.LoadFile, 'this/does/not/exist.xdd')").get<bool>());
}

TEST_F(LuaObjectDictionaryTest, Xdd_MalformedFilesAreErrors)
{
    for (const char* text : {"<a><b></a>", "<a>", "<a/>", "<a x=1/>"}) {
        lua["Text"] = std::string {text};
        EXPECT_FALSE(lua.script("return pcall(OdParser.Parse, Text)").get<bool>()) << text;
    }
}

TEST_F(LuaObjectDictionaryTest, Xdd_ExcessiveNestingIsAnError)
{
    std::string deep;
    for (int i = 0; i < 1000; ++i) {
        deep += "<a>";
    }
    lua["Text"] = deep;
    EXPECT_FALSE(lua.script("return pcall(OdParser.Parse, Text)").get<bool>());
}
