#pragma once
// =============================================================================
// Minimal jsoncpp-compatible header for test compilation
// Provides the subset of Json::Value, Json::Reader, Json::StreamWriterBuilder
// used by the kanba backend.
// =============================================================================

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <cstddef>

namespace Json {

enum ValueType {
    nullValue = 0,
    intValue,
    uintValue,
    realValue,
    stringValue,
    booleanValue,
    arrayValue,
    objectValue
};

// Forward declare
class Value;

// A simplified Json::Value
class Value {
public:
    Value() : type_(nullValue) {}
    explicit Value(ValueType type) : type_(type) {}
    Value(const char* s) : type_(stringValue), strVal_(s) {}
    Value(const std::string& s) : type_(stringValue), strVal_(s) {}
    Value(int v) : type_(intValue), intVal_(v) {}
    Value(unsigned v) : type_(uintValue), intVal_(static_cast<int>(v)) {}
    Value(double v) : type_(realValue), dblVal_(v) {}
    Value(bool v) : type_(booleanValue), boolVal_(v) {}

    // Type queries
    bool isNull() const { return type_ == nullValue; }
    bool isString() const { return type_ == stringValue; }
    bool isInt() const { return type_ == intValue; }
    bool isUInt() const { return type_ == uintValue; }
    bool isDouble() const { return type_ == realValue; }
    bool isBool() const { return type_ == booleanValue; }
    bool isArray() const { return type_ == arrayValue; }
    bool isObject() const { return type_ == objectValue || !members_.empty(); }

    // Conversions
    std::string asString() const { return strVal_; }
    int asInt() const { return intVal_; }
    unsigned asUInt() const { return static_cast<unsigned>(intVal_); }
    double asDouble() const { return dblVal_; }
    bool asBool() const { return boolVal_; }

    // Object access
    Value& operator[](const std::string& key) {
        if (type_ == nullValue) type_ = objectValue;
        return members_[key];
    }
    Value& operator[](const char* key) { return (*this)[std::string(key)]; }
    const Value& operator[](const std::string& key) const {
        static Value null;
        auto it = members_.find(key);
        return it != members_.end() ? it->second : null;
    }
    const Value& operator[](const char* key) const { return (*this)[std::string(key)]; }

    // Array access
    Value& operator[](int index) {
        if (type_ == nullValue) type_ = arrayValue;
        if (static_cast<size_t>(index) >= array_.size()) array_.resize(index + 1);
        return array_[index];
    }
    const Value& operator[](int index) const {
        static Value null;
        if (static_cast<size_t>(index) >= array_.size()) return null;
        return array_[index];
    }
    Value& operator[](unsigned index) { return (*this)[static_cast<int>(index)]; }
    const Value& operator[](unsigned index) const { return (*this)[static_cast<int>(index)]; }

    void append(const Value& v) {
        if (type_ == nullValue) type_ = arrayValue;
        array_.push_back(v);
    }

    size_t size() const {
        if (type_ == arrayValue) return array_.size();
        if (type_ == objectValue) return members_.size();
        return 0;
    }
    bool empty() const { return size() == 0 && type_ != objectValue; }

    bool isMember(const std::string& key) const {
        return members_.find(key) != members_.end();
    }

    std::vector<std::string> getMemberNames() const {
        std::vector<std::string> names;
        for (auto& p : members_) names.push_back(p.first);
        return names;
    }

    // Assignment operators
    Value& operator=(ValueType vt) { type_ = vt; return *this; }  // handles Json::nullValue etc.
    Value& operator=(const char* s) { type_ = stringValue; strVal_ = s; return *this; }
    Value& operator=(const std::string& s) { type_ = stringValue; strVal_ = s; return *this; }
    Value& operator=(int v) { type_ = intValue; intVal_ = v; return *this; }
    Value& operator=(unsigned v) { type_ = uintValue; intVal_ = static_cast<int>(v); return *this; }
    Value& operator=(bool v) { type_ = booleanValue; boolVal_ = v; return *this; }
    Value& operator=(double v) { type_ = realValue; dblVal_ = v; return *this; }
    Value& operator=(std::nullptr_t) { type_ = nullValue; return *this; }

    // Comparison
    bool operator==(const Value& other) const {
        if (type_ != other.type_) return false;
        switch (type_) {
            case nullValue: return true;
            case stringValue: return strVal_ == other.strVal_;
            case intValue: case uintValue: return intVal_ == other.intVal_;
            case realValue: return dblVal_ == other.dblVal_;
            case booleanValue: return boolVal_ == other.boolVal_;
            default: return false;
        }
    }
    bool operator!=(const Value& other) const { return !(*this == other); }

    // Serialization (minimal)
    std::string toStyledString() const { return serialize(); }

    static const Value nullSingleton;

private:
    std::string serialize() const {
        switch (type_) {
            case nullValue: return "null";
            case stringValue: return "\"" + strVal_ + "\"";
            case intValue: case uintValue: return std::to_string(intVal_);
            case realValue: return std::to_string(dblVal_);
            case booleanValue: return boolVal_ ? "true" : "false";
            case arrayValue: {
                std::string r = "[";
                for (size_t i = 0; i < array_.size(); i++) {
                    if (i) r += ",";
                    r += array_[i].serialize();
                }
                return r + "]";
            }
            case objectValue: {
                std::string r = "{";
                bool first = true;
                for (auto& p : members_) {
                    if (!first) r += ",";
                    r += "\"" + p.first + "\":" + p.second.serialize();
                    first = false;
                }
                return r + "}";
            }
        }
        return "null";
    }

    ValueType type_;
    std::string strVal_;
    int intVal_ = 0;
    double dblVal_ = 0.0;
    bool boolVal_ = false;
    std::map<std::string, Value> members_;
    std::vector<Value> array_;
};

inline const Value Value::nullSingleton;

// Minimal Reader
class Reader {
public:
    bool parse(const std::string& document, Value& root, bool = true) {
        // Very minimal JSON parser for test scenarios
        if (document.empty() || document == "null") { root = Value(); return true; }
        if (document == "[]") { root = Value(arrayValue); return true; }
        if (document == "{}") { root = Value(objectValue); return true; }
        if (document[0] == '[') {
            root = Value(arrayValue);
            // Simple array of strings: ["a","b"]
            size_t pos = 1;
            while (pos < document.size()) {
                if (document[pos] == '"') {
                    size_t end = document.find('"', pos + 1);
                    if (end != std::string::npos) {
                        root.append(Value(document.substr(pos + 1, end - pos - 1)));
                        pos = end + 1;
                    } else break;
                } else pos++;
            }
            return true;
        }
        return false;
    }
};

// Minimal StreamWriterBuilder + writeString
class StreamWriterBuilder {
public:
    Value& operator[](const std::string&) { static Value v; return v; }
};

inline std::string writeString(const StreamWriterBuilder&, const Value& val) {
    return val.toStyledString();
}

} // namespace Json
