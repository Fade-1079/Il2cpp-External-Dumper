#include "app.h"
#include <filesystem>
#include "dumper.h"
#include "imgui.h"
#include "writers.h"

namespace fs = std::filesystem;

App::~App() {
    if (th.joinable()) th.join();
}

void App::Log(const std::string& s) {
    std::lock_guard<std::mutex> l(mx);
    log.push_back(s);
}
std::atomic<bool> stop{ false };
void App::StartDump() {
    if (th.joinable()) th.join();
    busy = true;
    prog = 0.f;
    th = std::thread([this] {
        Dumper d(proc, api);
        std::vector<Cls> cs;
        if (d.Run(cs, prog, stop, [this](const std::string& s) {
            Log(s);
            })) {
            fs::path dir = out;
            fs::create_directories(dir);
            if (oDump) WriteDump((dir / "dump.cs").string(), cs);
            if (oBetter) WriteBetter((dir / "better_dump.cs").string(), cs);
            if (oOffsets) WriteOffsets((dir / "offsets.hpp").string(), cs);
            if (oCsv) WriteMethodsCsv((dir / "methods.csv").string(), cs);
            if (oList) WriteClassList((dir / "classes.txt").string(), cs);
            if (oIda) WriteIda((dir / "ida_names.py").string(), cs);
            if (oExp) WriteExports((dir / "exports.txt").string(), proc.Exports(), proc.Base());
            Log("written " + std::to_string(cs.size()) + " classes to " + fs::absolute(dir).string());
        }
        busy = false;
    });
}

void App::Draw() {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::Begin("main", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

    ImGui::SeparatorText("Target");
    ImGui::BeginDisabled(busy);
    ImGui::SetNextItemWidth(260);
    ImGui::InputText("Process", exe, sizeof(exe));
    ImGui::SameLine();
    if (ImGui::Button("Attach")) {
        api.Reset();
        wchar_t wExe[128];
        mbstowcs_s(nullptr, wExe, exe, _TRUNCATE);
        if (proc.Attach(wExe))
            Log("attached, GameAssembly base " + Hex(proc.Base()));
        else Log("attach failed");
    }
    ImGui::SameLine();
    if (ImGui::Button("Detach")) { proc.Detach(); Log("detached"); }
    ImGui::EndDisabled();
    ImGui::Text("State: %s", proc.Ok() ? "attached" : "idle");
    if (proc.Ok()) ImGui::Text("GameAssembly.dll  %s  size %s", Hex(proc.Base()).c_str(), Hex(proc.Size()).c_str());

    ImGui::SeparatorText("Output");
    ImGui::SetNextItemWidth(360);
    ImGui::InputText("Folder", out, sizeof(out));
    ImGui::Checkbox("dump.cs", &oDump); ImGui::SameLine();
    ImGui::Checkbox("better_dump.cs", &oBetter); ImGui::SameLine();
    ImGui::Checkbox("offsets.hpp", &oOffsets); ImGui::SameLine();
    ImGui::Checkbox("methods.csv", &oCsv);
    ImGui::Checkbox("classes.txt", &oList); ImGui::SameLine();
    ImGui::Checkbox("ida_names.py", &oIda); ImGui::SameLine();
    ImGui::Checkbox("exports.txt", &oExp);

    ImGui::Spacing();
    ImGui::BeginDisabled(busy || !proc.Ok());
    if (ImGui::Button("Dump", ImVec2(140, 32))) StartDump();
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::ProgressBar(prog, ImVec2(-1, 32));

    ImGui::SeparatorText("Log");
    ImGui::BeginChild("log", ImVec2(0, 0), ImGuiChildFlags_Borders);
    {
        std::lock_guard<std::mutex> l(mx);
        for (auto& s : log) ImGui::TextUnformatted(s.c_str());
    }
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.f);
    ImGui::EndChild();
    ImGui::End();
}
