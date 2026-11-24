#include <fstream>
#include <sstream>
#include "writers.h"

void WriteBetter(const std::string& path, const std::vector<Cls>& cs) {
    std::ofstream o(path);
    for (auto& c : cs) {
        o << "// " << c.image << "\n";
        o << (c.ns.empty() ? "" : "namespace " + c.ns + "\n") << ClsHead(c) << "\n{\n";
        for (auto& f : c.fields) {
            if (c.isEnum && f.hasConst) {
                o << "\t" << f.name << " = " << f.constVal << ",\n";
                continue;
            }
            o << "\t" << Acc(f.flags) << ((f.flags & 0x10) ? "static " : "") << f.type << " " << f.name << "; // "
              << Hex((uint32_t)f.offset) << "\n";
        }
        if (!c.fields.empty()) o << "\n";
        if (!c.isEnum) {
            for (auto& m : c.methods) {
                if (!m.ptr) continue;
                bool st = m.flags & 0x10;
                std::string args, sig;
                for (size_t i = 0; i < m.params.size(); i++) {
                    sig += (i ? ", " : "") + m.params[i].type;
                    args += "arg" + std::to_string(i) + ", ";
                }
                std::string full = m.ret + " " + m.name + "(" + sig + ")";
                std::string types = st ? "" : c.name + "*, ";
                for (auto& pr : m.params) types += pr.type + ", ";
                o << "\tRVA: " << Hex(m.rva) << " | Addr: " << Hex(m.ptr) << "\n";
                o << "\t" << full << "\n\t{\n";
                o << "\t\tMETHOD(" << m.ret << ", (" << types << "MethodInfo*), \"" << full << "\");\n";
                o << "\t\treturn function(" << (st ? "" : "this, ") << args << "nullptr);\n";
                o << "\t}\n\n";
            }
        }
        o << "}\n\n";
    }
}
