#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>

namespace {
    HMODULE g_module = nullptr;

    std::wstring ModuleDirectory() {
        wchar_t path[MAX_PATH]{};
        GetModuleFileNameW(g_module, path, MAX_PATH);
        std::wstring value(path);
        const auto slash = value.find_last_of(L"\\/");
        return slash == std::wstring::npos ? L"." : value.substr(0, slash);
    }

    void AppendLog(const std::wstring& line) {
        const auto path = ModuleDirectory() + L"\\StencilShadows.log";
        std::wofstream out(path, std::ios::app);
        if (out) out << line << L"\n";
    }

    DWORD WINAPI Initialize(LPVOID) {
        SYSTEMTIME now{};
        GetLocalTime(&now);

        std::wstringstream header;
        header << L"StencilShadows loaded at "
               << std::setfill(L'0')
               << std::setw(2) << now.wHour << L":"
               << std::setw(2) << now.wMinute << L":"
               << std::setw(2) << now.wSecond;
        AppendLog(header.str());
        AppendLog(L"Build: x86 ASI bootstrap; renderer integration is not implemented yet.");
        AppendLog(L"Configuration file: StencilShadows.ini");
        return 0;
    }
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_module = module;
        DisableThreadLibraryCalls(module);
        HANDLE thread = CreateThread(nullptr, 0, Initialize, nullptr, 0, nullptr);
        if (thread) CloseHandle(thread);
    }
    return TRUE;
}
