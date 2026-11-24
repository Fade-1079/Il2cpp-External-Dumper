#pragma once
#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include "il2cpp.h"
#include "proc.h"

class App {
public:
    ~App();
    void Draw();

private:
    void Log(const std::string& s);
    void StartDump();
    Proc proc;
    Il2cpp api{proc};
    char exe[128] = "RecRoom.exe";
    char out[260] = "dump_out";
    bool oDump = true, oBetter = true, oOffsets = true, oCsv = true, oList = true, oIda = true, oExp = true;
    std::atomic<bool> busy{false};
    std::atomic<float> prog{0.f};
    std::mutex mx;
    std::vector<std::string> log;
    std::thread th;
};
