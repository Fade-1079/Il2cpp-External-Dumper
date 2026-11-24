#include "dumper.h"

std::string Dumper::TypeName(uint64_t t) {
    if (!t) return "System.Void";
    auto it = tc.find(t);
    if (it != tc.end()) return it->second;
    std::string s = a.S(a.C("il2cpp_type_get_name", t));
    if (s.empty()) s = "System.Object";
    return tc[t] = s;
}

std::string Dumper::ClassName(uint64_t k) {
    if (!k) return "";
    auto it = cc.find(k);
    if (it != cc.end()) return it->second;
    std::string n = a.S(a.C("il2cpp_class_get_name", k)), r;
    uint64_t d = a.C("il2cpp_class_get_declaring_type", k);
    if (d && d != k) {
        r = ClassName(d) + "/" + n;
    }
    else {
        std::string ns = a.S(a.C("il2cpp_class_get_namespace", k));
        r = ns.empty() ? n : ns + "." + n;
    }
    return cc[k] = r;
}

static int64_t Extend(uint64_t v, int t) {
    switch (t) {
    case 4: return (int8_t)v;
    case 6: return (int16_t)v;
    case 8: return (int32_t)v;
    default: return (int64_t)v;
    }
}

Cls Dumper::Build(uint64_t k, const std::string& img) {
    Cls c;
    c.ptr = k;
    c.image = img;
    c.name = a.S(a.C("il2cpp_class_get_name", k));
    c.ns = a.S(a.C("il2cpp_class_get_namespace", k));
    c.full = ClassName(k);
    c.flags = (uint32_t)a.C("il2cpp_class_get_flags", k);
    c.size = (uint32_t)a.C("il2cpp_class_instance_size", k);
    c.isEnum = a.C("il2cpp_class_is_enum", k) & 0xFF;
    c.isValue = a.C("il2cpp_class_is_valuetype", k) & 0xFF;
    c.isIface = (c.flags & 0x20) != 0;

    uint64_t dt = a.C("il2cpp_class_get_declaring_type", k);
    if (dt && dt != k) {
        c.outer = ClassName(dt);
        uint64_t root = dt;
        for (int i = 0; i < 16; i++) {
            uint64_t up = a.C("il2cpp_class_get_declaring_type", root);
            if (!up || up == root) break;
            root = up;
        }
        c.ns = a.S(a.C("il2cpp_class_get_namespace", root));
    }

    uint64_t par = a.C("il2cpp_class_get_parent", k);
    if (par) c.parent = ClassName(par);
    int ebt = 8;
    if (c.isEnum) {
        uint64_t bt = a.C("il2cpp_class_enum_basetype", k);
        ebt = (int)(a.C("il2cpp_type_get_type", bt) & 0xFF);
        c.enumBase = TypeName(bt);
    }
    uint64_t it = p.Scratch() + 0x10, val = p.Scratch() + 0x40;

    p.Write<uint64_t>(it, 0);
    while (uint64_t i = a.C("il2cpp_class_get_interfaces", k, it)) c.ifaces.push_back(ClassName(i));

    p.Write<uint64_t>(it, 0);
    while (uint64_t f = a.C("il2cpp_class_get_fields", k, it)) {
        Fld x;
        x.name = a.S(a.C("il2cpp_field_get_name", f));
        x.type = TypeName(a.C("il2cpp_field_get_type", f));
        x.flags = (uint32_t)a.C("il2cpp_field_get_flags", f) & 0xFFFF;
        x.offset = (int32_t)a.C("il2cpp_field_get_offset", f);
        if (c.isEnum && (x.flags & 0x40)) {
            p.Write<uint64_t>(val, 0);
            a.C("il2cpp_field_static_get_value", f, val);
            x.constVal = Extend(p.Read<uint64_t>(val), ebt);
            x.hasConst = true;
        }
        c.fields.push_back(std::move(x));
    }

    p.Write<uint64_t>(it, 0);
    while (uint64_t m = a.C("il2cpp_class_get_methods", k, it)) {
        Mth x;
        x.mi = m;
        x.name = a.S(a.C("il2cpp_method_get_name", m));
        x.ptr = p.Read<uint64_t>(m);
        x.rva = x.ptr >= p.Base() ? x.ptr - p.Base() : 0;
        x.flags = (uint32_t)a.C("il2cpp_method_get_flags", m, 0) & 0xFFFF;
        x.ret = TypeName(a.C("il2cpp_method_get_return_type", m));
        uint32_t n = (uint32_t)a.C("il2cpp_method_get_param_count", m);
        if (n > 32) n = 0;
        for (uint32_t i = 0; i < n; i++) {
            Prm pr;
            pr.type = TypeName(a.C("il2cpp_method_get_param", m, i));
            pr.name = a.S(a.C("il2cpp_method_get_param_name", m, i));
            x.params.push_back(std::move(pr));
        }
        c.methods.push_back(std::move(x));
    }

    p.Write<uint64_t>(it, 0);
    while (uint64_t pr = a.C("il2cpp_class_get_properties", k, it)) {
        Prp x;
        x.name = a.S(a.C("il2cpp_property_get_name", pr));
        uint64_t g = a.C("il2cpp_property_get_get_method", pr);
        uint64_t s = a.C("il2cpp_property_get_set_method", pr);
        x.get = g != 0;
        x.set = s != 0;
        if (g) {
            x.flags = (uint32_t)a.C("il2cpp_method_get_flags", g, 0) & 0xFFFF;
            x.type = TypeName(a.C("il2cpp_method_get_return_type", g));
        }
        else if (s) {
            x.flags = (uint32_t)a.C("il2cpp_method_get_flags", s, 0) & 0xFFFF;
            x.type = TypeName(a.C("il2cpp_method_get_param", s, 0));
        }
        c.props.push_back(std::move(x));
    }
    return c;
}

bool Dumper::Run(std::vector<Cls>& out, std::atomic<float>& prog, std::atomic<bool>& stop,
    const std::function<void(const std::string&)>& log) {
    if (!a.Ready()) { log("il2cpp exports not found"); return false; }
    uint64_t dom = a.C("il2cpp_domain_get");
    if (!dom) { log("domain_get returned null"); return false; }
    a.C("il2cpp_thread_attach", dom);
    uint64_t cnt = p.Scratch() + 0x20;
    p.Write<uint64_t>(cnt, 0);
    uint64_t arr = a.C("il2cpp_domain_get_assemblies", dom, cnt);
    uint64_t n = p.Read<uint64_t>(cnt);
    if (!arr || !n || n > 4096) { log("no assemblies"); return false; }
    log("assemblies: " + std::to_string(n));

    struct Img { std::string name; uint64_t ptr; uint32_t count; };
    std::vector<Img> imgs;
    uint64_t total = 0;
    for (uint64_t i = 0; i < n; i++) {
        uint64_t as = p.Read<uint64_t>(arr + i * 8);
        uint64_t im = a.C("il2cpp_assembly_get_image", as);
        if (!im) continue;
        Img x{ a.S(a.C("il2cpp_image_get_name", im)), im, (uint32_t)a.C("il2cpp_image_get_class_count", im) };
        total += x.count;
        imgs.push_back(x);
    }
    log("classes: " + std::to_string(total));
    uint64_t done = 0;
    for (auto& im : imgs) {
        for (uint32_t j = 0; j < im.count; j++, done++) {
            if (stop) { log("cancelled"); return false; }
            uint64_t k = a.C("il2cpp_image_get_class", im.ptr, j);
            if (k) out.push_back(Build(k, im.name));
            if ((done & 31) == 0) prog = total ? (float)done / (float)total : 1.f;
        }
        log(im.name + "  " + std::to_string(im.count) + " classes");
    }
    prog = 1.f;
    return true;
}