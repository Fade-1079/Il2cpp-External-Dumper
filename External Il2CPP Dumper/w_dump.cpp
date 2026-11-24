#include <fstream>
#include "writers.h"

void WriteDump(const std::string& path, const std::vector<Cls>& cs) {
    std::ofstream o(path);
    std::string img;
    for (auto& c : cs) {
        if (c.image != img) {
            img = c.image;
            o << "// Image: " << img << "\n\n";
        }
        o << "// Namespace: " << c.ns << "\n";
        o << ClsHead(c) << "\n{\n";
        if (c.isEnum) {
            for (auto& f : c.fields) {
                if (f.hasConst) o << "\t" << f.name << " = " << f.constVal << ",\n";
            }
        } else {
            if (!c.fields.empty()) o << "\t// Fields\n";
            for (auto& f : c.fields) {
                o << "\t" << Acc(f.flags) << ((f.flags & 0x40) ? "const " : (f.flags & 0x10) ? "static " : "")
                  << ((f.flags & 0x20) && !(f.flags & 0x40) ? "readonly " : "") << Short(f.type) << " " << f.name
                  << "; // " << Hex((uint32_t)f.offset) << "\n";
            }
            if (!c.props.empty()) o << "\n\t// Properties\n";
            for (auto& p : c.props) {
                o << "\t" << Acc(p.flags) << ((p.flags & 0x10) ? "static " : "") << Short(p.type) << " " << p.name
                  << " { " << (p.get ? "get; " : "") << (p.set ? "set; " : "") << "}\n";
            }
            if (!c.methods.empty()) o << "\n\t// Methods\n";
            for (auto& m : c.methods) {
                o << "\t// RVA: " << Hex(m.rva) << " VA: " << Hex(m.ptr) << "\n";
                o << "\t" << Acc(m.flags) << ((m.flags & 0x10) ? "static " : "") << ((m.flags & 0x400) ? "abstract " : (m.flags & 0x40) ? "virtual " : "")
                  << Short(m.ret) << " " << m.name << "(";
                for (size_t i = 0; i < m.params.size(); i++) {
                    o << (i ? ", " : "") << Short(m.params[i].type) << " " << m.params[i].name;
                }
                o << ") { }\n\n";
            }
        }
        o << "}\n\n";
    }
}
