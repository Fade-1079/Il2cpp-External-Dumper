#pragma once
#include <atomic>
#include <functional>
#include <unordered_map>
#include <vector>
#include "il2cpp.h"
#include "model.h"

class Dumper {
public:
    Dumper(Proc& p, Il2cpp& a) : p(p), a(a) {}
    bool Run(std::vector<Cls>& out, std::atomic<float>& prog, std::atomic<bool>& stop,
        const std::function<void(const std::string&)>& log);

private:
    Cls Build(uint64_t k, const std::string& img);
    std::string TypeName(uint64_t t);
    std::string ClassName(uint64_t k);
    Proc& p;
    Il2cpp& a;
    std::unordered_map<uint64_t, std::string> tc, cc;
};