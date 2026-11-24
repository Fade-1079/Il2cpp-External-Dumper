#pragma once
#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include "model.h"

void WriteDump(const std::string& path, const std::vector<Cls>& cs);
void WriteBetter(const std::string& path, const std::vector<Cls>& cs);
void WriteOffsets(const std::string& path, const std::vector<Cls>& cs);
void WriteMethodsCsv(const std::string& path, const std::vector<Cls>& cs);
void WriteClassList(const std::string& path, const std::vector<Cls>& cs);
void WriteIda(const std::string& path, const std::vector<Cls>& cs);
void WriteExports(const std::string& path, const std::map<std::string, uint64_t>& e, uint64_t base);

inline std::string Hex(uint64_t v) {
    char b[32];
    snprintf(b, sizeof(b), "0x%llX", (unsigned long long)v);
    return b;
}

inline std::string Short(const std::string& s) {
    static const char* m[][2] = {{"System.Int32", "int"}, {"System.Int64", "long"}, {"System.Int16", "short"},
                                 {"System.UInt32", "uint"}, {"System.UInt64", "ulong"}, {"System.UInt16", "ushort"},
                                 {"System.Byte", "byte"}, {"System.SByte", "sbyte"}, {"System.Single", "float"},
                                 {"System.Double", "double"}, {"System.Boolean", "bool"}, {"System.String", "string"},
                                 {"System.Object", "object"}, {"System.Void", "void"}, {"System.Char", "char"}};
    for (auto& e : m) if (s == e[0]) return e[1];
    return s;
}

inline const char* Acc(uint32_t f) {
    switch (f & 7) {
        case 1: return "private ";
        case 2: return "private protected ";
        case 3: return "internal ";
        case 4: return "protected ";
        case 5: return "protected internal ";
        case 6: return "public ";
    }
    return "";
}

inline std::string ClsMods(const Cls& c) {
    std::string s;
    switch (c.flags & 7) {
        case 0: s = "internal "; break;
        case 1: case 2: s = "public "; break;
        case 3: s = "private "; break;
        case 4: s = "protected "; break;
        case 5: s = "internal "; break;
        case 6: s = "private protected "; break;
        case 7: s = "protected internal "; break;
    }
    bool ab = c.flags & 0x80, se = c.flags & 0x100;
    if (c.isEnum || c.isIface || c.isValue) return s;
    if (ab && se) s += "static ";
    else if (ab) s += "abstract ";
    else if (se) s += "sealed ";
    return s;
}

inline std::string ClsHead(const Cls& c) {
    std::string kind = c.isEnum ? "enum " : c.isIface ? "interface " : c.isValue ? "struct " : "class ";
    std::string s = ClsMods(c) + kind + c.name;
    std::string bases;
    if (c.isEnum) bases = Short(c.enumBase);
    else if (!c.isValue && !c.parent.empty()) bases = c.parent;
    for (auto& i : c.ifaces) bases += (bases.empty() ? "" : ", ") + i;
    if (!bases.empty()) s += " : " + bases;
    return s;
}

inline std::string Ident(std::string s) {
    for (auto& ch : s) if (!isalnum((unsigned char)ch) && ch != '_') ch = '_';
    if (s.empty() || isdigit((unsigned char)s[0])) s = "_" + s;
    return s;
}
