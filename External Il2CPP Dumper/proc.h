#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>
#include <TlHelp32.h>
#include <cstdint>
#include <filesystem>
#include <map>
#include <string>

class Proc {
public:
    ~Proc() { Detach(); }
    bool Attach(DWORD target);
    bool Attach(const wchar_t* processName);
    void Detach();
    bool Ok() const { return h && worker; }
    DWORD Pid() const { return pid; }
    uint64_t Base() const { return base; }
    uint64_t Size() const { return size; }
    uint64_t Scratch() const { return mem + 0x200; }
    bool ReadRaw(uint64_t a, void* b, size_t n) const;
    bool WriteRaw(uint64_t a, const void* b, size_t n) const;
    template <class T> T Read(uint64_t a) const { T v{}; ReadRaw(a, &v, sizeof(T)); return v; }
    template <class T> void Write(uint64_t a, const T& v) const { WriteRaw(a, &v, sizeof(T)); }
    std::string ReadStr(uint64_t a, size_t max = 2048) const;
    const std::map<std::string, uint64_t>& Exports();
    uint64_t Export(const std::string& n);
    uint64_t Call(uint64_t fn, uint64_t a = 0, uint64_t b = 0, uint64_t c = 0, uint64_t d = 0);
    bool DumpImage(const std::filesystem::path& path) const;

private:
    bool StartWorker();
    HANDLE h = nullptr, worker = nullptr;
    DWORD pid = 0;
    uint64_t base = 0, size = 0, mem = 0, data = 0;
    std::map<std::string, uint64_t> exports;
};
