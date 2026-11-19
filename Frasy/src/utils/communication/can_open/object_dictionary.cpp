/**
 * @file    object_dictionary.cpp
 * @author  Paul Thomas
 * @date    2026-11-19
 * @brief   Description of the object dictionary of a CANopen device, loaded from its EDS or XDD file.
 *
 * @copyright
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this
 * program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "object_dictionary.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <format>
#include <fstream>
#include <map>
#include <optional>
#include <sstream>
#include <unordered_map>
#include <utility>

namespace Frasy::CanOpen {

namespace {

// --- Generic helpers ---

//! Files bigger than this are not object dictionary descriptions.
constexpr std::uintmax_t s_maxFileSize = 16ull * 1024 * 1024;

std::string_view trim(std::string_view text)
{
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front()))) {
        text.remove_prefix(1);
    }
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back()))) {
        text.remove_suffix(1);
    }
    return text;
}

std::string toLower(std::string_view text)
{
    std::string out(text);
    std::ranges::transform(out, out.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

bool isHex(std::string_view text)
{
    return !text.empty() && std::ranges::all_of(text, [](unsigned char c) { return std::isxdigit(c) != 0; });
}

std::optional<uint32_t> parseHex(std::string_view text)
{
    text = trim(text);
    if (text.size() > 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) { text.remove_prefix(2); }
    uint32_t value = 0;
    auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value, 16);
    if (text.empty() || ec != std::errc {} || ptr != text.data() + text.size()) { return std::nullopt; }
    return value;
}

//! Parses a decimal number, or a hexadecimal one when prefixed with 0x.
std::optional<uint32_t> parseNumber(std::string_view text)
{
    text = trim(text);
    if (text.size() > 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) { return parseHex(text); }
    uint32_t value = 0;
    auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value, 10);
    if (text.empty() || ec != std::errc {} || ptr != text.data() + text.size()) { return std::nullopt; }
    return value;
}

// --- CiA 301 data types ---

struct DataTypeInfo {
    uint16_t         code;
    std::string_view name;
    VarType          varType;
};

// Types whose size has no native C++ counterpart (24, 40, 48 and 56 bits integers, ...) are handled as raw data.
constexpr std::array s_dataTypes = {
    DataTypeInfo {0x0001, "BOOLEAN", VarType::Boolean},
    DataTypeInfo {0x0002, "INTEGER8", VarType::Signed8},
    DataTypeInfo {0x0003, "INTEGER16", VarType::Signed16},
    DataTypeInfo {0x0004, "INTEGER32", VarType::Signed32},
    DataTypeInfo {0x0005, "UNSIGNED8", VarType::Unsigned8},
    DataTypeInfo {0x0006, "UNSIGNED16", VarType::Unsigned16},
    DataTypeInfo {0x0007, "UNSIGNED32", VarType::Unsigned32},
    DataTypeInfo {0x0008, "REAL32", VarType::Real32},
    DataTypeInfo {0x0009, "VISIBLE_STRING", VarType::String},
    DataTypeInfo {0x000A, "OCTET_STRING", VarType::Undefined},
    DataTypeInfo {0x000B, "UNICODE_STRING", VarType::Undefined},
    DataTypeInfo {0x000C, "TIME_OF_DAY", VarType::Undefined},
    DataTypeInfo {0x000D, "TIME_DIFFERENCE", VarType::Undefined},
    DataTypeInfo {0x000F, "DOMAIN", VarType::Undefined},
    DataTypeInfo {0x0010, "INTEGER24", VarType::Undefined},
    DataTypeInfo {0x0011, "REAL64", VarType::Real64},
    DataTypeInfo {0x0012, "INTEGER40", VarType::Undefined},
    DataTypeInfo {0x0013, "INTEGER48", VarType::Undefined},
    DataTypeInfo {0x0014, "INTEGER56", VarType::Undefined},
    DataTypeInfo {0x0015, "INTEGER64", VarType::Signed64},
    DataTypeInfo {0x0016, "UNSIGNED24", VarType::Undefined},
    DataTypeInfo {0x0018, "UNSIGNED40", VarType::Undefined},
    DataTypeInfo {0x0019, "UNSIGNED48", VarType::Undefined},
    DataTypeInfo {0x001A, "UNSIGNED56", VarType::Undefined},
    DataTypeInfo {0x001B, "UNSIGNED64", VarType::Unsigned64},
};

const DataTypeInfo* findDataType(uint16_t code)
{
    auto it = std::ranges::find(s_dataTypes, code, &DataTypeInfo::code);
    return it == s_dataTypes.end() ? nullptr : &*it;
}

//! Names of the data types in a XDD, which are XML elements, mapped to the code of the type.
struct XddTypeInfo {
    std::string_view element;
    uint16_t         code;
};

constexpr std::array s_xddTypes = {
    XddTypeInfo {"BOOL", 0x0001},
    XddTypeInfo {"SINT", 0x0002},
    XddTypeInfo {"INT", 0x0003},
    XddTypeInfo {"DINT", 0x0004},
    XddTypeInfo {"LINT", 0x0015},
    XddTypeInfo {"USINT", 0x0005},
    XddTypeInfo {"UINT", 0x0006},
    XddTypeInfo {"UDINT", 0x0007},
    XddTypeInfo {"ULINT", 0x001B},
    XddTypeInfo {"BYTE", 0x0005},
    XddTypeInfo {"CHAR", 0x0005},
    XddTypeInfo {"WORD", 0x0006},
    XddTypeInfo {"DWORD", 0x0007},
    XddTypeInfo {"LWORD", 0x001B},
    XddTypeInfo {"REAL", 0x0008},
    XddTypeInfo {"LREAL", 0x0011},
    XddTypeInfo {"STRING", 0x0009},
    XddTypeInfo {"WSTRING", 0x000B},
    XddTypeInfo {"BITSTRING", 0x000A},
    XddTypeInfo {"OCTET_STRING", 0x000A},
    XddTypeInfo {"VISIBLE_STRING", 0x0009},
    XddTypeInfo {"DOMAIN", 0x000F},
};

std::optional<uint16_t> findXddType(std::string_view elementName)
{
    auto it = std::ranges::find(s_xddTypes, elementName, &XddTypeInfo::element);
    return it == s_xddTypes.end() ? std::nullopt : std::optional {it->code};
}

void setDataType(OdEntry& entry, uint16_t code)
{
    entry.dataType = code;
    if (const auto* info = findDataType(code); info != nullptr) {
        entry.varType      = info->varType;
        entry.dataTypeName = info->name;
    }
    else {
        entry.varType      = VarType::Undefined;
        entry.dataTypeName = std::format("0x{:04X}", code);
    }
}

OdAccess parseAccess(std::string_view text)
{
    const auto lower = toLower(trim(text));
    if (lower == "ro") { return OdAccess::ReadOnly; }
    if (lower == "wo") { return OdAccess::WriteOnly; }
    if (lower == "rw" || lower == "rwr" || lower == "rww") { return OdAccess::ReadWrite; }
    if (lower == "const") { return OdAccess::Const; }
    return OdAccess::ReadOnly;
}

OdAccess parseXddAccess(std::string_view text)
{
    const auto lower = toLower(trim(text));
    if (lower == "const") { return OdAccess::Const; }
    if (lower == "write") { return OdAccess::WriteOnly; }
    if (lower == "readwrite" || lower == "readwriteinput" || lower == "readwriteoutput") {
        return OdAccess::ReadWrite;
    }
    if (lower == "noaccess") { return OdAccess::None; }
    return OdAccess::ReadOnly;
}

void sortObject(OdObject& object)
{
    std::ranges::sort(object.entries, {}, &OdEntry::subIndex);
}

// --- EDS (INI) ---

struct IniSection {
    std::string                                       name;
    std::vector<std::pair<std::string, std::string>> keys;

    [[nodiscard]] std::optional<std::string_view> get(std::string_view key) const
    {
        auto it = std::ranges::find(keys, key, &std::pair<std::string, std::string>::first);
        return it == keys.end() ? std::nullopt : std::optional<std::string_view> {it->second};
    }
};

std::vector<IniSection> parseIni(std::string_view text)
{
    std::vector<IniSection> sections;
    std::istringstream      stream {std::string {text}};
    std::string             rawLine;
    while (std::getline(stream, rawLine)) {
        auto line = trim(rawLine);
        if (line.starts_with("\xEF\xBB\xBF")) { line = trim(line.substr(3)); }    // UTF-8 byte order mark.
        if (line.empty() || line.front() == ';') { continue; }

        if (line.front() == '[') {
            const auto end = line.find(']');
            if (end == std::string_view::npos) { continue; }
            sections.push_back({.name = toLower(trim(line.substr(1, end - 1))), .keys = {}});
            continue;
        }

        const auto equal = line.find('=');
        if (equal == std::string_view::npos || sections.empty()) { continue; }
        sections.back().keys.emplace_back(toLower(trim(line.substr(0, equal))), std::string {trim(line.substr(equal + 1))});
    }
    return sections;
}

//! Section names are `XXXX` for an object and `XXXXsubY` for one of its sub-indexes (lowered by the parser).
std::optional<uint16_t> parseObjectSectionName(std::string_view name)
{
    if (name.size() != 4 || !isHex(name)) { return std::nullopt; }
    return static_cast<uint16_t>(*parseHex(name));
}

std::optional<std::pair<uint16_t, uint8_t>> parseSubObjectSectionName(std::string_view name)
{
    if (name.size() < 8 || name.size() > 9 || name.substr(4, 3) != "sub") { return std::nullopt; }
    const auto index = name.substr(0, 4);
    const auto sub   = name.substr(7);
    if (!isHex(index) || !isHex(sub)) { return std::nullopt; }
    return std::pair {static_cast<uint16_t>(*parseHex(index)), static_cast<uint8_t>(*parseHex(sub))};
}

OdEntry makeEdsEntry(const IniSection& section, uint8_t subIndex)
{
    OdEntry entry;
    entry.subIndex = subIndex;
    if (auto v = section.get("parametername")) { entry.name = *v; }
    if (auto v = section.get("datatype"); v && parseNumber(*v)) { setDataType(entry, static_cast<uint16_t>(*parseNumber(*v))); }
    if (auto v = section.get("accesstype")) { entry.access = parseAccess(*v); }
    if (auto v = section.get("defaultvalue")) { entry.defaultValue = *v; }
    if (auto v = section.get("pdomapping")) { entry.isPdoMappable = parseNumber(*v).value_or(0) != 0; }
    if (auto v = section.get("objecttype"); v && parseNumber(*v)) {
        entry.objectType = static_cast<OdObjectType>(*parseNumber(*v));
    }
    if (auto v = section.get("lowlimit")) { entry.lowLimit = *v; }
    if (auto v = section.get("highlimit")) { entry.highLimit = *v; }
    if (auto v = section.get("stringlengthmin")) { entry.stringLengthMin = parseNumber(*v); }
    return entry;
}

// --- XML ---

struct XmlNode {
    std::string                                       name;
    std::vector<std::pair<std::string, std::string>> attributes;
    std::string                                       text;
    std::vector<XmlNode>                              children;

    [[nodiscard]] const std::string* attribute(std::string_view key) const
    {
        auto it = std::ranges::find(attributes, key, &std::pair<std::string, std::string>::first);
        return it == attributes.end() ? nullptr : &it->second;
    }

    [[nodiscard]] std::string_view attributeOr(std::string_view key, std::string_view fallback = {}) const
    {
        const auto* value = attribute(key);
        return value == nullptr ? fallback : std::string_view {*value};
    }

    [[nodiscard]] const XmlNode* child(std::string_view childName) const
    {
        auto it = std::ranges::find(children, childName, &XmlNode::name);
        return it == children.end() ? nullptr : &*it;
    }
};

struct XmlError {
    std::string message;
};

/**
 * Minimal XML reader, sufficient for the XDD files. Namespace prefixes are dropped from the names (`q1:parameter`
 * becomes `parameter`). There is no validation, DTDs are ignored.
 */
class XmlParser {
public:
    explicit XmlParser(std::string_view text) : m_text(text) {}

    XmlNode parseDocument()
    {
        if (m_text.starts_with("\xEF\xBB\xBF")) { m_pos = 3; }
        skipMisc();
        if (atEnd() || peek() != '<') { fail("no root element"); }
        auto root = parseElement(0);
        skipMisc();
        return root;
    }

private:
    static constexpr int s_maxDepth = 128;

    [[noreturn]] void fail(std::string_view what) const
    {
        throw XmlError {std::format("XML error at offset {}: {}", m_pos, what)};
    }

    [[nodiscard]] bool atEnd() const { return m_pos >= m_text.size(); }
    [[nodiscard]] char peek() const { return m_text[m_pos]; }
    [[nodiscard]] bool startsWith(std::string_view s) const { return m_text.substr(m_pos).starts_with(s); }

    void skipWhitespace()
    {
        while (!atEnd() && std::isspace(static_cast<unsigned char>(peek()))) {
            ++m_pos;
        }
    }

    void skipUntilAfter(std::string_view terminator)
    {
        const auto end = m_text.find(terminator, m_pos);
        if (end == std::string_view::npos) { fail(std::format("missing '{}'", terminator)); }
        m_pos = end + terminator.size();
    }

    //! Skips whitespace, comments, processing instructions and the DOCTYPE.
    void skipMisc()
    {
        while (true) {
            skipWhitespace();
            if (startsWith("<!--")) { skipUntilAfter("-->"); }
            else if (startsWith("<?")) {
                skipUntilAfter("?>");
            }
            else if (startsWith("<!DOCTYPE")) {
                skipUntilAfter(">");
            }
            else {
                return;
            }
        }
    }

    std::string parseName()
    {
        const auto start = m_pos;
        while (!atEnd() && !std::isspace(static_cast<unsigned char>(peek())) && peek() != '=' && peek() != '>' &&
               peek() != '/') {
            ++m_pos;
        }
        if (m_pos == start) { fail("expected a name"); }
        std::string name {m_text.substr(start, m_pos - start)};
        if (const auto colon = name.rfind(':'); colon != std::string::npos) { name.erase(0, colon + 1); }
        return name;
    }

    static void appendUtf8(std::string& out, uint32_t codePoint)
    {
        if (codePoint < 0x80) { out += static_cast<char>(codePoint); }
        else if (codePoint < 0x800) {
            out += static_cast<char>(0xC0 | (codePoint >> 6));
            out += static_cast<char>(0x80 | (codePoint & 0x3F));
        }
        else if (codePoint < 0x10000) {
            out += static_cast<char>(0xE0 | (codePoint >> 12));
            out += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (codePoint & 0x3F));
        }
        else {
            out += static_cast<char>(0xF0 | (codePoint >> 18));
            out += static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F));
            out += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (codePoint & 0x3F));
        }
    }

    static std::string decodeEntities(std::string_view raw)
    {
        std::string out;
        out.reserve(raw.size());
        for (std::size_t i = 0; i < raw.size(); ++i) {
            if (raw[i] != '&') {
                out += raw[i];
                continue;
            }
            const auto end = raw.find(';', i);
            if (end == std::string_view::npos) {
                out += raw[i];
                continue;
            }
            const auto entity = raw.substr(i + 1, end - i - 1);
            if (entity == "lt") { out += '<'; }
            else if (entity == "gt") {
                out += '>';
            }
            else if (entity == "amp") {
                out += '&';
            }
            else if (entity == "quot") {
                out += '"';
            }
            else if (entity == "apos") {
                out += '\'';
            }
            else if (entity.starts_with('#') && entity.size() > 1) {
                const bool hex  = entity[1] == 'x' || entity[1] == 'X';
                const auto digits = entity.substr(hex ? 2 : 1);
                uint32_t   codePoint = 0;
                auto [ptr, ec] = std::from_chars(digits.data(), digits.data() + digits.size(), codePoint, hex ? 16 : 10);
                if (digits.empty() || ec != std::errc {} || ptr != digits.data() + digits.size() || codePoint > 0x10FFFF) {
                    out += raw.substr(i, end - i + 1);
                }
                else {
                    appendUtf8(out, codePoint);
                }
            }
            else {
                out += raw.substr(i, end - i + 1);    // Unknown entity, keep it as is.
            }
            i = end;
        }
        return out;
    }

    XmlNode parseElement(int depth)
    {
        if (depth > s_maxDepth) { fail("elements are nested too deeply"); }
        ++m_pos;    // '<'
        XmlNode node;
        node.name = parseName();

        // Attributes
        while (true) {
            skipWhitespace();
            if (atEnd()) { fail("unterminated tag"); }
            if (startsWith("/>")) {
                m_pos += 2;
                return node;
            }
            if (peek() == '>') {
                ++m_pos;
                break;
            }
            auto key = parseName();
            skipWhitespace();
            if (atEnd() || peek() != '=') { fail("expected '=' after attribute name"); }
            ++m_pos;
            skipWhitespace();
            if (atEnd() || (peek() != '"' && peek() != '\'')) { fail("expected a quoted attribute value"); }
            const char quote = peek();
            ++m_pos;
            const auto end = m_text.find(quote, m_pos);
            if (end == std::string_view::npos) { fail("unterminated attribute value"); }
            node.attributes.emplace_back(std::move(key), decodeEntities(m_text.substr(m_pos, end - m_pos)));
            m_pos = end + 1;
        }

        // Content
        while (true) {
            if (atEnd()) { fail(std::format("element <{}> is never closed", node.name)); }
            if (startsWith("</")) {
                m_pos += 2;
                const auto closing = parseName();
                skipWhitespace();
                if (atEnd() || peek() != '>') { fail("malformed closing tag"); }
                ++m_pos;
                if (closing != node.name) { fail(std::format("<{}> closed by </{}>", node.name, closing)); }
                return node;
            }
            if (startsWith("<!--")) { skipUntilAfter("-->"); }
            else if (startsWith("<![CDATA[")) {
                m_pos += 9;
                const auto end = m_text.find("]]>", m_pos);
                if (end == std::string_view::npos) { fail("unterminated CDATA"); }
                node.text += m_text.substr(m_pos, end - m_pos);
                m_pos = end + 3;
            }
            else if (startsWith("<?")) {
                skipUntilAfter("?>");
            }
            else if (peek() == '<') {
                node.children.push_back(parseElement(depth + 1));
            }
            else {
                const auto end = m_text.find('<', m_pos);
                const auto stop = end == std::string_view::npos ? m_text.size() : end;
                node.text += decodeEntities(m_text.substr(m_pos, stop - m_pos));
                m_pos = stop;
            }
        }
    }

    std::string_view m_text;
    std::size_t      m_pos = 0;
};

//! Depth-first search of every node with the given name.
void collect(const XmlNode& node, std::string_view name, std::vector<const XmlNode*>& out)
{
    if (node.name == name) { out.push_back(&node); }
    for (const auto& child : node.children) {
        collect(child, name, out);
    }
}

const XmlNode* findFirst(const XmlNode& node, std::string_view name)
{
    std::vector<const XmlNode*> found;
    collect(node, name, found);
    return found.empty() ? nullptr : found.front();
}

std::string trimmedText(const XmlNode* node)
{
    return node == nullptr ? std::string {} : std::string {trim(node->text)};
}

//! Everything needed to resolve the references between the elements of a XDD.
struct XddContext {
    std::unordered_map<std::string, const XmlNode*> parameters;
    std::unordered_map<std::string, const XmlNode*> dataTypes;

    [[nodiscard]] const XmlNode* findParameter(const XmlNode& object) const
    {
        const auto* ref = object.attribute("uniqueIDRef");
        if (ref == nullptr) { return nullptr; }
        auto it = parameters.find(*ref);
        return it == parameters.end() ? nullptr : it->second;
    }

    /**
     * Finds the data type declared by an element (a parameter, or an array, enumeration or derived type), following
     * the references to the type definitions.
     */
    [[nodiscard]] std::optional<uint16_t> resolveType(const XmlNode& node, int depth = 0) const
    {
        if (depth > 8) { return std::nullopt; }
        for (const auto& child : node.children) {
            if (child.name == "dataTypeIDRef") {
                const auto* ref = child.attribute("uniqueIDRef");
                if (ref == nullptr) { continue; }
                auto it = dataTypes.find(*ref);
                if (it == dataTypes.end()) { continue; }
                if (auto code = resolveType(*it->second, depth + 1)) { return code; }
            }
            else if (auto code = findXddType(child.name)) {
                return code;
            }
        }
        return std::nullopt;
    }

    //! Finds the type of a member of a structure, from the name of the member.
    [[nodiscard]] std::optional<uint16_t> resolveStructMemberType(const XmlNode& parameter, std::string_view member) const
    {
        for (const auto& child : parameter.children) {
            if (child.name != "dataTypeIDRef") { continue; }
            const auto* ref = child.attribute("uniqueIDRef");
            if (ref == nullptr) { continue; }
            auto it = dataTypes.find(*ref);
            if (it == dataTypes.end() || it->second->name != "struct") { continue; }
            for (const auto& declaration : it->second->children) {
                if (declaration.name == "varDeclaration" && declaration.attributeOr("name") == member) {
                    return resolveType(declaration, 1);
                }
            }
        }
        return std::nullopt;
    }
};

bool isPdoMappable(std::string_view value)
{
    const auto lower = toLower(trim(value));
    return !lower.empty() && lower != "no";
}

/**
 * Builds an entry from a CANopenObject (when it is a variable) or a CANopenSubObject.
 *
 * @param parent Parameter of the enclosing object, which supplies a type to the sub-indexes that do not declare one.
 */
OdEntry makeXddEntry(const XmlNode& node, uint8_t subIndex, const XddContext& context, const XmlNode* parent)
{
    OdEntry entry;
    entry.subIndex = subIndex;

    const XmlNode* parameter = context.findParameter(node);
    entry.name               = node.attributeOr("name");
    if (entry.name.empty() && parameter != nullptr) { entry.name = trimmedText(parameter->child("label")); }

    std::optional<uint16_t> code;
    if (parameter != nullptr) { code = context.resolveType(*parameter); }
    if (!code && parent != nullptr) {
        code = context.resolveStructMemberType(*parent, entry.name);
        if (!code) { code = context.resolveType(*parent); }
    }
    if (!code) {
        if (auto v = parseHex(node.attributeOr("dataType"))) { code = static_cast<uint16_t>(*v); }
    }
    if (code) { setDataType(entry, *code); }

    if (node.attribute("accessType") != nullptr) { entry.access = parseAccess(node.attributeOr("accessType")); }
    else if (parameter != nullptr && parameter->attribute("access") != nullptr) {
        entry.access = parseXddAccess(parameter->attributeOr("access"));
    }
    else {
        entry.access = OdAccess::ReadOnly;    // "read" is the default of the `access` attribute.
    }

    if (parameter != nullptr) {
        if (const auto* def = parameter->child("defaultValue")) { entry.defaultValue = def->attributeOr("value"); }
        entry.description = trimmedText(parameter->child("description"));
    }
    if (entry.defaultValue.empty()) { entry.defaultValue = node.attributeOr("defaultValue"); }

    if (parameter != nullptr) {
        if (const auto* allowed = parameter->child("allowedValues")) {
            if (const auto* range = allowed->child("range")) {
                if (const auto* min = range->child("minValue")) { entry.lowLimit = min->attributeOr("value"); }
                if (const auto* max = range->child("maxValue")) { entry.highLimit = max->attributeOr("value"); }
            }
        }
        // The XDD equivalent of the EDS `StringLengthMin`, used to pad the strings when they are downloaded.
        for (const auto& property : parameter->children) {
            if (property.name == "property" && property.attributeOr("name") == "CO_stringLengthMin") {
                entry.stringLengthMin = parseNumber(property.attributeOr("value"));
            }
        }
    }
    if (auto v = parseNumber(node.attributeOr("objectType"))) { entry.objectType = static_cast<OdObjectType>(*v); }

    entry.isPdoMappable = isPdoMappable(node.attributeOr("PDOmapping"));
    return entry;
}

}    // namespace

std::string_view toString(OdAccess access) noexcept
{
    switch (access) {
        case OdAccess::None: return "none";
        case OdAccess::ReadOnly: return "ro";
        case OdAccess::WriteOnly: return "wo";
        case OdAccess::ReadWrite: return "rw";
        case OdAccess::Const: return "const";
    }
    return "unknown";
}

std::string_view toString(OdObjectType type) noexcept
{
    switch (type) {
        case OdObjectType::Null: return "NULL";
        case OdObjectType::Domain: return "DOMAIN";
        case OdObjectType::DefType: return "DEFTYPE";
        case OdObjectType::DefStruct: return "DEFSTRUCT";
        case OdObjectType::Var: return "VAR";
        case OdObjectType::Array: return "ARRAY";
        case OdObjectType::Record: return "RECORD";
    }
    return "unknown";
}

const OdObject* ObjectDictionary::findObject(uint16_t index) const noexcept
{
    auto it = std::ranges::find(m_objects, index, &OdObject::index);
    return it == m_objects.end() ? nullptr : &*it;
}

std::expected<ObjectDictionary, std::string> ObjectDictionary::load(const std::filesystem::path& path)
{
    std::error_code ec;
    const auto      size = std::filesystem::file_size(path, ec);
    if (ec) { return std::unexpected {std::format("Unable to open '{}': {}", path.string(), ec.message())}; }
    if (size > s_maxFileSize) { return std::unexpected {std::format("'{}' is too big to be an EDS or XDD", path.string())}; }

    std::ifstream file {path, std::ios::binary};
    if (!file) { return std::unexpected {std::format("Unable to open '{}'", path.string())}; }
    std::string content(size, '\0');
    file.read(content.data(), static_cast<std::streamsize>(size));
    content.resize(static_cast<std::size_t>(file.gcount()));

    std::string_view view {content};
    if (view.starts_with("\xEF\xBB\xBF")) { view.remove_prefix(3); }
    view = trim(view);
    return view.starts_with('<') ? parseXdd(content) : parseEds(content);
}

std::expected<ObjectDictionary, std::string> ObjectDictionary::parse(std::string_view text)
{
    if (text.starts_with("\xEF\xBB\xBF")) { text.remove_prefix(3); }
    return trim(text).starts_with('<') ? parseXdd(text) : parseEds(text);
}

std::expected<ObjectDictionary, std::string> ObjectDictionary::parseEds(std::string_view text)
{
    const auto sections = parseIni(text);

    ObjectDictionary od;
    std::map<uint16_t, OdObject>                                      objects;
    std::map<uint16_t, std::map<uint8_t, const IniSection*>>          subSections;
    std::map<uint16_t, const IniSection*>                             objectSections;

    for (const auto& section : sections) {
        if (section.name == "deviceinfo") {
            if (auto v = section.get("vendorname")) { od.m_vendorName = *v; }
            if (auto v = section.get("productname")) { od.m_productName = *v; }
        }
        else if (auto index = parseObjectSectionName(section.name)) {
            objectSections[*index] = &section;
        }
        else if (auto sub = parseSubObjectSectionName(section.name)) {
            subSections[sub->first][sub->second] = &section;
        }
    }

    for (const auto& [index, section] : objectSections) {
        OdObject object;
        object.index = index;
        if (auto v = section->get("parametername")) { object.name = *v; }
        object.type = static_cast<OdObjectType>(parseNumber(section->get("objecttype").value_or("7")).value_or(7));

        if (object.hasSubIndexes()) {
            if (auto it = subSections.find(index); it != subSections.end()) {
                for (const auto& [subIndex, subSection] : it->second) {
                    object.entries.push_back(makeEdsEntry(*subSection, subIndex));
                }
            }
            else if (auto count = parseNumber(section->get("compactsubobj").value_or("")); count && *count < 256) {
                // Compact notation: every sub-index shares the definition of the object.
                OdEntry length;
                length.subIndex     = 0;
                length.name         = "Number of entries";
                length.varType      = VarType::Unsigned8;
                length.dataTypeName = "UNSIGNED8";
                length.access       = OdAccess::ReadOnly;
                object.entries.push_back(length);
                for (uint32_t i = 1; i <= *count; ++i) {
                    auto entry     = makeEdsEntry(*section, static_cast<uint8_t>(i));
                    entry.name       = std::format("{} {}", object.name, i);
                    entry.objectType = OdObjectType::Var;    // The key of the object is the one of the array.
                    object.entries.push_back(std::move(entry));
                }
            }
        }
        else {
            object.entries.push_back(makeEdsEntry(*section, 0));
        }
        object.subNumber = parseNumber(section->get("subnumber").value_or("")).value_or(
          static_cast<uint32_t>(object.entries.size()));
        sortObject(object);
        od.m_objects.push_back(std::move(object));
    }

    if (od.m_objects.empty()) { return std::unexpected {"The file does not describe any object (not an EDS file?)"}; }
    return od;
}

std::expected<ObjectDictionary, std::string> ObjectDictionary::parseXdd(std::string_view text)
{
    XmlNode root;
    try {
        root = XmlParser {text}.parseDocument();
    }
    catch (const XmlError& e) {
        return std::unexpected {e.message};
    }

    XddContext                  context;
    std::vector<const XmlNode*> found;
    collect(root, "parameter", found);
    for (const auto* parameter : found) {
        if (const auto* id = parameter->attribute("uniqueID")) { context.parameters.emplace(*id, parameter); }
    }
    for (const char* kind : {"array", "struct", "enum", "derived"}) {
        found.clear();
        collect(root, kind, found);
        for (const auto* type : found) {
            if (const auto* id = type->attribute("uniqueID")) { context.dataTypes.emplace(*id, type); }
        }
    }

    const auto* objectList = findFirst(root, "CANopenObjectList");
    if (objectList == nullptr) { return std::unexpected {"The file does not contain a CANopenObjectList (not a XDD file?)"}; }

    ObjectDictionary od;
    od.m_vendorName  = trimmedText(findFirst(root, "vendorName"));
    od.m_productName = trimmedText(findFirst(root, "productName"));

    for (const auto& node : objectList->children) {
        if (node.name != "CANopenObject") { continue; }
        const auto index = parseHex(node.attributeOr("index"));
        if (!index || *index > 0xFFFF) { continue; }

        OdObject object;
        object.index = static_cast<uint16_t>(*index);
        object.name  = node.attributeOr("name");
        object.type  = static_cast<OdObjectType>(parseNumber(node.attributeOr("objectType", "7")).value_or(7));

        const XmlNode* parameter = context.findParameter(node);
        if (parameter != nullptr) {
            object.description = trimmedText(parameter->child("description"));
            if (object.name.empty()) { object.name = trimmedText(parameter->child("label")); }
        }

        if (object.hasSubIndexes()) {
            for (const auto& sub : node.children) {
                if (sub.name != "CANopenSubObject") { continue; }
                const auto subIndex = parseHex(sub.attributeOr("subIndex"));
                if (!subIndex || *subIndex > 0xFF) { continue; }
                object.entries.push_back(makeXddEntry(sub, static_cast<uint8_t>(*subIndex), context, parameter));
            }
        }
        else {
            object.entries.push_back(makeXddEntry(node, 0, context, nullptr));
        }
        object.subNumber = parseNumber(node.attributeOr("subNumber")).value_or(
          static_cast<uint32_t>(object.entries.size()));
        sortObject(object);
        od.m_objects.push_back(std::move(object));
    }

    if (od.m_objects.empty()) { return std::unexpected {"The file does not describe any object"}; }
    std::ranges::sort(od.m_objects, {}, &OdObject::index);
    return od;
}

}    // namespace Frasy::CanOpen
