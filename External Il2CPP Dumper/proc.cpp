#include "proc.h"
#include <tlhelp32.h>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <vector>

static const uint8_t kStub[] = {
    0x53, 0x48, 0x83, 0xEC, 0x20,
    0x48, 0xBB, 0, 0, 0, 0, 0, 0, 0, 0,
    0x48, 0x8B, 0x43, 0x30,
    0x48, 0x83, 0xF8, 0x01,
    0x74, 0x0A,
    0x48, 0x83, 0xF8, 0x03,
    0x74, 0x27,
    0xF3, 0x90,
    0xEB, 0xEC,
    0x48, 0x8B, 0x03,
    0x48, 0x8B, 0x4B, 0x08,
    0x48, 0x8B, 0x53, 0x10,
    0x4C, 0x8B, 0x43, 0x18,
    0x4C, 0x8B, 0x4B, 0x20,
    0xFF, 0xD0,
    0x48, 0x89, 0x43, 0x28,
    0x48, 0xC7, 0x43, 0x30, 0x02, 0x00, 0x00, 0x00,
    0xEB, 0xC9,
    0x48, 0x83, 0xC4, 0x20,
    0x5B,
    0x31, 0xC0,
    0xC3 };
bool Proc::Attach(const wchar_t* processName)
{
    if (!processName || !*processName)
        return false;

    HANDLE snapshot =
        CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (snapshot == INVALID_HANDLE_VALUE)
        return false;

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);

    DWORD target = 0;

    if (Process32FirstW(snapshot, &entry))
    {
        do
        {
            if (_wcsicmp(entry.szExeFile, processName) == 0)
            {
                target = entry.th32ProcessID;
                break;
            }
        } while (Process32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);

    return target != 0 && Attach(target);
}
bool Proc::Attach(DWORD target) {
    Detach();
    h = OpenProcess(PROCESS_ALL_ACCESS, FALSE, target);
    if (!h) return false;
    pid = target;
    HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (s != INVALID_HANDLE_VALUE) {
        MODULEENTRY32W me{};
        me.dwSize = sizeof(me);
        if (Module32FirstW(s, &me)) {
            do {
                if (!_wcsicmp(me.szModule, L"GameAssembly.dll")) {
                    base = (uint64_t)me.modBaseAddr;
                    size = me.modBaseSize;
                    break;
                }
            } while (Module32NextW(s, &me));
        }
        CloseHandle(s);
    }
    if (!base || !StartWorker()) { Detach(); return false; }
    return true;
}

bool Proc::StartWorker() {
    mem = (uint64_t)VirtualAllocEx(h, nullptr, 0x2000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!mem) return false;
    data = mem + 0x100;
    std::vector<uint8_t> st(kStub, kStub + sizeof(kStub));
    memcpy(&st[7], &data, 8);
    uint8_t zero[64]{};
    WriteRaw(data, zero, sizeof(zero));
    WriteRaw(mem, st.data(), st.size());
    worker = CreateRemoteThread(h, nullptr, 0, (LPTHREAD_START_ROUTINE)mem, nullptr, 0, nullptr);
    return worker != nullptr;
}

void Proc::Detach() {
    if (h && worker) {
        Write<uint64_t>(data + 48, 3);
        WaitForSingleObject(worker, 2000);
    }
    if (worker) CloseHandle(worker);
    if (h && mem) VirtualFreeEx(h, (LPVOID)mem, 0, MEM_RELEASE);
    if (h) CloseHandle(h);
    h = worker = nullptr;
    pid = 0;
    base = size = mem = data = 0;
    exports.clear();
}

bool Proc::ReadRaw(uint64_t a, void* b, size_t n) const {
    SIZE_T r = 0;
    return ReadProcessMemory(h, (LPCVOID)a, b, n, &r) && r == n;
}

bool Proc::WriteRaw(uint64_t a, const void* b, size_t n) const {
    SIZE_T r = 0;
    return WriteProcessMemory(h, (LPVOID)a, b, n, &r) && r == n;
}

std::string Proc::ReadStr(uint64_t a, size_t max) const {
    std::string s;
    if (!a) return s;
    char buf[256];
    while (s.size() < max) {
        size_t n = std::min<size_t>(sizeof(buf), 0x1000 - (a & 0xFFF));
        if (!ReadRaw(a, buf, n)) break;
        size_t l = strnlen(buf, n);
        s.append(buf, l);
        if (l < n) break;
        a += n;
    }
    return s;
}

const std::map<std::string, uint64_t>& Proc::Exports() {
    if (!exports.empty() || !h) return exports;
    auto dos = Read<IMAGE_DOS_HEADER>(base);
    auto nt = Read<IMAGE_NT_HEADERS64>(base + dos.e_lfanew);
    auto dir = nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (!dir.VirtualAddress) return exports;
    auto ed = Read<IMAGE_EXPORT_DIRECTORY>(base + dir.VirtualAddress);
    if (ed.NumberOfNames > 200000 || ed.NumberOfFunctions > 200000) return exports;
    std::vector<uint32_t> names(ed.NumberOfNames), funcs(ed.NumberOfFunctions);
    std::vector<uint16_t> ords(ed.NumberOfNames);
    ReadRaw(base + ed.AddressOfNames, names.data(), names.size() * 4);
    ReadRaw(base + ed.AddressOfNameOrdinals, ords.data(), ords.size() * 2);
    ReadRaw(base + ed.AddressOfFunctions, funcs.data(), funcs.size() * 4);
    for (uint32_t i = 0; i < ed.NumberOfNames; i++) {
        if (ords[i] >= funcs.size()) continue;
        exports[ReadStr(base + names[i], 256)] = base + funcs[ords[i]];
    }
    return exports;
}

uint64_t Proc::Export(const std::string& n) {
    auto& e = Exports();
    auto it = e.find(n);
    return it == e.end() ? 0 : it->second;
}

uint64_t Proc::Call(uint64_t fn, uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
    uint64_t blk[5] = { fn, a, b, c, d };
    WriteRaw(data, blk, sizeof(blk));
    Write<uint64_t>(data + 48, 1);
    ULONGLONG t0 = GetTickCount64();
    while (Read<uint64_t>(data + 48) != 2) {
        if (GetTickCount64() - t0 > 8000) return 0;
        if (WaitForSingleObject(worker, 0) == WAIT_OBJECT_0) return 0;
    }
    uint64_t r = Read<uint64_t>(data + 40);
    Write<uint64_t>(data + 48, 0);
    return r;
}

bool Proc::DumpImage(const std::filesystem::path& path) const {
    if (!h || !base || !size) return false;
    std::vector<uint8_t> img((size_t)size);
    const size_t chunk = 0x10000;
    for (size_t o = 0; o < img.size(); o += chunk) {
        size_t n = (std::min)(chunk, img.size() - o);
        if (ReadRaw(base + o, &img[o], n)) continue;
        for (size_t q = 0; q < n; q += 0x1000)
            ReadRaw(base + o + q, &img[o + q], std::min<size_t>(0x1000, n - q));
    }
    if (img.size() < sizeof(IMAGE_DOS_HEADER)) return false;
    auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(img.data());
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return false;
    if ((size_t)dos->e_lfanew + sizeof(IMAGE_NT_HEADERS64) > img.size()) return false;
    auto nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(img.data() + dos->e_lfanew);
    auto sec = IMAGE_FIRST_SECTION(nt);
    for (WORD i = 0; i < nt->FileHeader.NumberOfSections; i++) {
        sec[i].PointerToRawData = sec[i].VirtualAddress;
        uint64_t len = sec[i].Misc.VirtualSize;
        if ((uint64_t)sec[i].VirtualAddress + len > img.size()) len = img.size() - sec[i].VirtualAddress;
        sec[i].SizeOfRawData = (DWORD)len;
    }
    nt->OptionalHeader.ImageBase = base;
    nt->OptionalHeader.FileAlignment = nt->OptionalHeader.SectionAlignment;
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    std::ofstream o(path, std::ios::binary);
    if (!o) return false;
    o.write(reinterpret_cast<const char*>(img.data()), (std::streamsize)img.size());
    return o.good();
}