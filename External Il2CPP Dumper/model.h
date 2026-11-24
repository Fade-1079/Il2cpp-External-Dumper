#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct Fld {
    std::string name, type;
    uint32_t flags = 0;
    int32_t offset = 0;
    bool hasConst = false;
    int64_t constVal = 0;
};

struct Prm {
    std::string type, name;
};

struct Mth {
    std::string name, ret;
    std::vector<Prm> params;
    uint32_t flags = 0;
    uint64_t mi = 0, ptr = 0, rva = 0;
};

struct Prp {
    std::string name, type;
    uint32_t flags = 0;
    bool get = false, set = false;
};

struct Cls {
    std::string image, ns, name, full, parent, enumBase, outer;
    std::vector<std::string> ifaces;
    std::vector<Fld> fields;
    std::vector<Mth> methods;
    std::vector<Prp> props;
    uint32_t flags = 0, size = 0;
    bool isEnum = false, isValue = false, isIface = false;
    uint64_t ptr = 0;
};