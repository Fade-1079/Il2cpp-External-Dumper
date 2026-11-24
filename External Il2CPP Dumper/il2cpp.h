#pragma once
#include <string>
#include <unordered_map>
#include "proc.h"

class Il2cpp {
public:
    explicit Il2cpp(Proc& p) : p(p) {}
    uint64_t C(const char* n, uint64_t a = 0, uint64_t b = 0, uint64_t c = 0, uint64_t d = 0) {
        auto it = fn.find(n);
        if (it == fn.end()) it = fn.emplace(n, p.Export(n)).first;
        return it->second ? p.Call(it->second, a, b, c, d) : 0;
    }
    std::string S(uint64_t ptr) { return p.ReadStr(ptr); }
    bool Ready() { return p.Export("il2cpp_domain_get") != 0; }
    void Reset() { fn.clear(); }

private:
    Proc& p;
    std::unordered_map<std::string, uint64_t> fn;
};