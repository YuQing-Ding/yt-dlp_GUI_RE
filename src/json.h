// json.h —— 够用就好的 JSON 解析器，专门用来啃 yt-dlp -J 的输出。
// 只读、递归下降、UTF-8 进 UTF-8 出。不追求通用，追求不出错。
#pragma once

#include <map>
#include <string>
#include <vector>

namespace json {

struct Value;
using Object = std::map<std::string, Value>;
using Array  = std::vector<Value>;

struct Value {
    enum Type { Null, Bool, Num, Str, Arr, Obj };

    Type   type = Null;
    bool   b    = false;
    double num  = 0;
    std::string str;
    Array  arr;
    Object obj;

    bool isNull() const { return type == Null; }

    const Value* find(const std::string& key) const {
        if (type != Obj) return nullptr;
        auto it = obj.find(key);
        return it == obj.end() ? nullptr : &it->second;
    }

    std::string getStr(const std::string& key, const std::string& def = "") const {
        const Value* v = find(key);
        if (!v) return def;
        if (v->type == Str) return v->str;
        if (v->type == Num) {
            // yt-dlp 里有些字段（比如 format_id）可能是数字，统一成字符串好处理
            char buf[32];
            if (v->num == (long long)v->num) sprintf_s(buf, "%lld", (long long)v->num);
            else sprintf_s(buf, "%g", v->num);
            return buf;
        }
        return def;
    }

    double getNum(const std::string& key, double def = 0) const {
        const Value* v = find(key);
        return (v && v->type == Num) ? v->num : def;
    }

    bool getBool(const std::string& key, bool def = false) const {
        const Value* v = find(key);
        return (v && v->type == Bool) ? v->b : def;
    }

    const Array* getArr(const std::string& key) const {
        const Value* v = find(key);
        return (v && v->type == Arr) ? &v->arr : nullptr;
    }
};

namespace detail {

inline void SkipWs(const std::string& s, size_t& i) {
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\r')) ++i;
}

inline void EncodeUtf8(unsigned cp, std::string& out) {
    if (cp < 0x80) {
        out += (char)cp;
    } else if (cp < 0x800) {
        out += (char)(0xC0 | (cp >> 6));
        out += (char)(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        out += (char)(0xE0 | (cp >> 12));
        out += (char)(0x80 | ((cp >> 6) & 0x3F));
        out += (char)(0x80 | (cp & 0x3F));
    } else {
        out += (char)(0xF0 | (cp >> 18));
        out += (char)(0x80 | ((cp >> 12) & 0x3F));
        out += (char)(0x80 | ((cp >> 6) & 0x3F));
        out += (char)(0x80 | (cp & 0x3F));
    }
}

inline bool Hex4(const std::string& s, size_t i, unsigned& out) {
    if (i + 4 > s.size()) return false;
    out = 0;
    for (int k = 0; k < 4; ++k) {
        char c = s[i + k];
        out <<= 4;
        if (c >= '0' && c <= '9') out |= (unsigned)(c - '0');
        else if (c >= 'a' && c <= 'f') out |= (unsigned)(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') out |= (unsigned)(c - 'A' + 10);
        else return false;
    }
    return true;
}

inline bool ParseString(const std::string& s, size_t& i, std::string& out) {
    if (i >= s.size() || s[i] != '"') return false;
    ++i;
    out.clear();
    while (i < s.size()) {
        char c = s[i];
        if (c == '"') { ++i; return true; }
        if (c != '\\') { out += c; ++i; continue; }

        ++i;
        if (i >= s.size()) return false;
        char e = s[i++];
        switch (e) {
            case '"':  out += '"';  break;
            case '\\': out += '\\'; break;
            case '/':  out += '/';  break;
            case 'b':  out += '\b'; break;
            case 'f':  out += '\f'; break;
            case 'n':  out += '\n'; break;
            case 'r':  out += '\r'; break;
            case 't':  out += '\t'; break;
            case 'u': {
                unsigned cp = 0;
                if (!Hex4(s, i, cp)) return false;
                i += 4;
                // 代理对
                if (cp >= 0xD800 && cp <= 0xDBFF && i + 6 <= s.size() &&
                    s[i] == '\\' && s[i + 1] == 'u') {
                    unsigned lo = 0;
                    if (Hex4(s, i + 2, lo) && lo >= 0xDC00 && lo <= 0xDFFF) {
                        cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                        i += 6;
                    }
                }
                EncodeUtf8(cp, out);
                break;
            }
            default: return false;
        }
    }
    return false;
}

bool ParseValue(const std::string& s, size_t& i, Value& out, int depth);

inline bool ParseNumber(const std::string& s, size_t& i, Value& out) {
    size_t start = i;
    if (i < s.size() && (s[i] == '-' || s[i] == '+')) ++i;
    while (i < s.size() && ((s[i] >= '0' && s[i] <= '9') || s[i] == '.' ||
                            s[i] == 'e' || s[i] == 'E' || s[i] == '-' || s[i] == '+')) ++i;
    if (i == start) return false;
    try {
        out.type = Value::Num;
        out.num  = std::stod(s.substr(start, i - start));
    } catch (...) {
        return false;
    }
    return true;
}

inline bool ParseValue(const std::string& s, size_t& i, Value& out, int depth) {
    if (depth > 200) return false;  // 防止恶意深嵌套把栈爆掉
    SkipWs(s, i);
    if (i >= s.size()) return false;

    char c = s[i];
    if (c == '"') {
        out.type = Value::Str;
        return ParseString(s, i, out.str);
    }
    if (c == '{') {
        ++i;
        out.type = Value::Obj;
        SkipWs(s, i);
        if (i < s.size() && s[i] == '}') { ++i; return true; }
        while (i < s.size()) {
            SkipWs(s, i);
            std::string key;
            if (!ParseString(s, i, key)) return false;
            SkipWs(s, i);
            if (i >= s.size() || s[i] != ':') return false;
            ++i;
            Value v;
            if (!ParseValue(s, i, v, depth + 1)) return false;
            out.obj.emplace(std::move(key), std::move(v));
            SkipWs(s, i);
            if (i >= s.size()) return false;
            if (s[i] == ',') { ++i; continue; }
            if (s[i] == '}') { ++i; return true; }
            return false;
        }
        return false;
    }
    if (c == '[') {
        ++i;
        out.type = Value::Arr;
        SkipWs(s, i);
        if (i < s.size() && s[i] == ']') { ++i; return true; }
        while (i < s.size()) {
            Value v;
            if (!ParseValue(s, i, v, depth + 1)) return false;
            out.arr.push_back(std::move(v));
            SkipWs(s, i);
            if (i >= s.size()) return false;
            if (s[i] == ',') { ++i; continue; }
            if (s[i] == ']') { ++i; return true; }
            return false;
        }
        return false;
    }
    if (s.compare(i, 4, "true") == 0)  { i += 4; out.type = Value::Bool; out.b = true;  return true; }
    if (s.compare(i, 5, "false") == 0) { i += 5; out.type = Value::Bool; out.b = false; return true; }
    if (s.compare(i, 4, "null") == 0)  { i += 4; out.type = Value::Null; return true; }
    return ParseNumber(s, i, out);
}

}  // namespace detail

inline bool Parse(const std::string& text, Value& out) {
    size_t i = 0;
    // yt-dlp 偶尔会在 JSON 前面吐一行警告，从第一个 { 或 [ 开始找
    while (i < text.size() && text[i] != '{' && text[i] != '[') ++i;
    if (i >= text.size()) return false;
    return detail::ParseValue(text, i, out, 0);
}

}  // namespace json
