#include <fstream>
#include <map>
#include "writers.h"

void WriteOffsets(const std::string& path, const std::vector<Cls>& cs) {
    std::ofstream o(path);
    o << "#pragma once\n#include <cstdint>\n\nnamespace Offsets {\n";
    std::map<std::string, int> used;
    for (auto& c : cs) {
        std::string ns = Ident(c.full);
        int n = used[ns]++;
        if (n) ns += "_" + std::to_string(n);
        o << "namespace " << ns << " {\n";
        std::map<std::string, int> seen;
        for (auto& f : c.fields) {
            if (f.flags & 0x40) continue;
            std::string id = "F_" + Ident(f.name);
            int k = seen[id]++;
            if (k) id += "_" + std::to_string(k);
            o << "    constexpr uint32_t " << id << " = " << Hex((uint32_t)f.offset) << ";\n";
        }
        for (auto& m : c.methods) {
            if (!m.rva) continue;
            std::string id = "M_" + Ident(m.name);
            int k = seen[id]++;
            if (k) id += "_" + std::to_string(k);
            o << "    constexpr uint64_t " << id << " = " << Hex(m.rva) << ";\n";
        }
        o << "}\n";
    }
    o << "}\n";
}

void WriteMethodsCsv(const std::string& path, const std::vector<Cls>& cs) {
    std::ofstream o(path);
    o << "rva,va,image,class,method,params,return\n";
    for (auto& c : cs) {
        for (auto& m : c.methods) {
            std::string ps;
            for (size_t i = 0; i < m.params.size(); i++) ps += (i ? ";" : "") + m.params[i].type;
            o << Hex(m.rva) << "," << Hex(m.ptr) << "," << c.image << "," << c.full << "," << m.name << ",\"" << ps << "\"," << m.ret << "\n";
        }
    }
}

void WriteClassList(const std::string& path, const std::vector<Cls>& cs) {
    std::ofstream o(path);
    for (auto& c : cs) {
        o << c.image << " | " << c.full << " | " << Hex(c.ptr) << " | fields=" << c.fields.size()
          << " methods=" << c.methods.size() << " props=" << c.props.size() << "\n";
    }
}

void WriteIda(const std::string& path, const std::vector<Cls>& cs) {
    std::ofstream o(path);
    o << "import idaapi\nimport idc\n\nbase = idaapi.get_imagebase()\nN = [\n";
    for (auto& c : cs) {
        std::map<std::string, int> seen;
        for (auto& m : c.methods) {
            if (!m.rva) continue;
            std::string id = Ident(c.full) + "__" + Ident(m.name);
            int k = seen[id]++;
            if (k) id += "_" + std::to_string(k);
            o << "(" << Hex(m.rva) << ",\"" << id << "\"),\n";
        }
    }
    o << "]\nfor r, n in N:\n    idc.set_name(base + r, n, idc.SN_NOWARN | idc.SN_NOCHECK)\n";
}

void WriteExports(const std::string& path, const std::map<std::string, uint64_t>& e, uint64_t base) {
    std::ofstream o(path);
    for (auto& [n, a] : e) o << n << " | " << Hex(a - base) << " | " << Hex(a) << "\n";
}
