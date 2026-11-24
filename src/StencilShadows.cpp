#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <MinHook.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cwchar>
#include <fstream>
#include <iomanip>
#include <cstring>
#include <initializer_list>
#include <sstream>
#include <string>

namespace {
    HMODULE g_module = nullptr;
    volatile LONG g_running = 1;
    volatile LONG g_strengthBits = 0x3F800000;
    bool g_shadowHookInstalled = false;
    using StoreShadowFn = void(__cdecl*)(unsigned char, void*, const void*, float, float, float, float, short, unsigned char, unsigned char, unsigned char, float, bool, float, void*, bool);
    StoreShadowFn g_originalStoreShadow = nullptr;

    struct ShadowConfig {
        bool enabled = true;
        int maxShadows = 128;
        bool trees = true, streetLamps = true, bridges = true;
        bool player = true, pedestrians = true, dynamicFade = true;
        float dayStartHour = 6.0f, fullStrengthHour = 12.0f;
        float fadeOutStartHour = 18.0f, nightStrength = 0.12f;
        int updateIntervalMs = 100;
        float maxShadowDistance = 120.0f;
    };

    std::wstring ModuleDirectory() {
        wchar_t path[MAX_PATH]{};
        GetModuleFileNameW(g_module, path, MAX_PATH);
        std::wstring value(path);
        const auto slash = value.find_last_of(L"\\/");
        return slash == std::wstring::npos ? L"." : value.substr(0, slash);
    }

    std::wstring ModuleFile(const wchar_t* name) {
        return ModuleDirectory() + L"\\" + name;
    }

    void AppendLog(const std::wstring& line) {
        std::wofstream out(ModuleFile(L"StencilShadows.log"), std::ios::app);
        if (out) out << line << L"\n";
    }

    int ReadInt(const std::wstring& file, const wchar_t* section,
                const wchar_t* key, int fallback) {
        return GetPrivateProfileIntW(section, key, fallback, file.c_str());
    }

    float ReadFloat(const std::wstring& file, const wchar_t* section,
                    const wchar_t* key, float fallback) {
        wchar_t buffer[64]{}, fallbackText[64]{};
        swprintf_s(fallbackText, L"%.4f", fallback);
        GetPrivateProfileStringW(section, key, fallbackText, buffer,
                                 static_cast<DWORD>(_countof(buffer)), file.c_str());
        wchar_t* end = nullptr;
        const double value = wcstod(buffer, &end);
        if (end == buffer || !std::isfinite(value)) return fallback;
        return static_cast<float>(value);
    }

    ShadowConfig LoadConfig() {
        ShadowConfig cfg;
        const auto file = ModuleFile(L"StencilShadows.ini");
        cfg.enabled = ReadInt(file, L"General", L"Enabled", 1) != 0;
        cfg.maxShadows = std::clamp(ReadInt(file, L"General", L"MaxShadows", 128), 1, 128);

        cfg.trees = ReadInt(file, L"ShadowTargets", L"Trees", 1) != 0;
        cfg.streetLamps = ReadInt(file, L"ShadowTargets", L"StreetLamps", 1) != 0;
        cfg.bridges = ReadInt(file, L"ShadowTargets", L"Bridges", 1) != 0;
        cfg.player = ReadInt(file, L"ShadowTargets", L"Player", 1) != 0;
        cfg.pedestrians = ReadInt(file, L"ShadowTargets", L"Pedestrians", 1) != 0;

        cfg.dynamicFade = ReadInt(file, L"DynamicFade", L"Enabled", 1) != 0;
        cfg.dayStartHour = std::clamp(ReadFloat(file, L"DynamicFade", L"DayStartHour", 6.0f), 0.0f, 23.99f);
        cfg.fullStrengthHour = std::clamp(ReadFloat(file, L"DynamicFade", L"FullStrengthHour", 12.0f), 0.0f, 23.99f);
        cfg.fadeOutStartHour = std::clamp(ReadFloat(file, L"DynamicFade", L"FadeOutStartHour", 18.0f), 0.0f, 23.99f);
        cfg.nightStrength = std::clamp(ReadFloat(file, L"DynamicFade", L"NightStrength", 0.12f), 0.0f, 1.0f);
        cfg.updateIntervalMs = std::clamp(ReadInt(file, L"Performance", L"UpdateIntervalMs", 100), 16, 1000);
        cfg.maxShadowDistance = std::clamp(ReadFloat(file, L"Performance", L"MaxShadowDistance", 120.0f), 20.0f, 500.0f);

        if (cfg.fullStrengthHour <= cfg.dayStartHour)
            cfg.fullStrengthHour = std::min(23.99f, cfg.dayStartHour + 0.1f);
        if (cfg.fadeOutStartHour <= cfg.fullStrengthHour)
            cfg.fadeOutStartHour = std::min(23.99f, cfg.fullStrengthHour + 0.1f);
        return cfg;
    }

    float SmoothStep(float t) {
        t = std::clamp(t, 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

    // Fractional GTA in-game hour [0, 24); awaiting connection to CClock.
    float CalculateTimeStrength(const ShadowConfig& cfg, float gameHour) {
        if (!cfg.dynamicFade) return 1.0f;
        gameHour = std::fmod(gameHour, 24.0f);
        if (gameHour < 0.0f) gameHour += 24.0f;
        if (gameHour < cfg.dayStartHour) return cfg.nightStrength;
        if (gameHour < cfg.fullStrengthHour) {
            const float t = (gameHour - cfg.dayStartHour) /
                            (cfg.fullStrengthHour - cfg.dayStartHour);
            return cfg.nightStrength + (1.0f - cfg.nightStrength) * SmoothStep(t);
        }
        if (gameHour < cfg.fadeOutStartHour) return 1.0f;
        const float t = (gameHour - cfg.fadeOutStartHour) /
                        (24.0f - cfg.fadeOutStartHour);
        return 1.0f + (cfg.nightStrength - 1.0f) * SmoothStep(t);
    }

    void LogConfig(const ShadowConfig& cfg) {
        std::wstringstream line;
        line << L"Config loaded: enabled=" << cfg.enabled
             << L", maxShadows=" << cfg.maxShadows
             << L", updateIntervalMs=" << cfg.updateIntervalMs
             << L", maxDistance=" << cfg.maxShadowDistance;
        AppendLog(line.str());

        std::wstringstream targets;
        targets << L"Targets: trees=" << cfg.trees << L", streetLamps=" << cfg.streetLamps
                << L", bridges=" << cfg.bridges << L", player=" << cfg.player
                << L", pedestrians=" << cfg.pedestrians;
        AppendLog(targets.str());

        std::wstringstream fade;
        fade << L"Fade schedule: " << cfg.dayStartHour << L":00 -> "
             << cfg.fullStrengthHour << L":00 -> " << cfg.fadeOutStartHour
             << L":00; nightStrength=" << cfg.nightStrength;
        AppendLog(fade.str());

        for (const float hour : {0.0f, 6.0f, 9.0f, 12.0f, 18.0f, 21.0f}) {
            std::wstringstream sample;
            sample << L"Fade sample at " << hour << L":00 = "
                   << std::fixed << std::setprecision(3)
                   << CalculateTimeStrength(cfg, hour);
            AppendLog(sample.str());
        }
    }

    float CurrentTimeStrength() {
        LONG bits = InterlockedCompareExchange(&g_strengthBits, 0, 0);
        float value = 1.0f;
        std::memcpy(&value, &bits, sizeof(value));
        return std::clamp(value, 0.0f, 1.0f);
    }

    void __cdecl HookStoreShadowToBeRendered(
        unsigned char type, void* texture, const void* posn,
        float topX, float topY, float rightX, float rightY,
        short intensity, unsigned char red, unsigned char green,
        unsigned char blue, float zDistance, bool drawOnWater,
        float scale, void* realTimeShadow, bool drawOnBuildings) {
        if (g_originalStoreShadow) {
            short adjustedIntensity = intensity;
            if (intensity > 0) {
                const long scaled = std::lround(static_cast<double>(intensity) *
                                                static_cast<double>(CurrentTimeStrength()));
                adjustedIntensity = static_cast<short>(std::clamp(scaled, 0L, 32767L));
            }
            g_originalStoreShadow(type, texture, posn, topX, topY, rightX, rightY,
                                  adjustedIntensity, red, green, blue, zDistance,
                                  drawOnWater, scale, realTimeShadow, drawOnBuildings);
        }
    }

    bool InstallShadowIntensityHook() {
        constexpr uintptr_t kStoreShadowAddress = 0x00707390; // CShadows::StoreShadowToBeRendered, SA 1.0 US
        const MH_STATUS initStatus = MH_Initialize();
        if (initStatus != MH_OK && initStatus != MH_ERROR_ALREADY_INITIALIZED) {
            std::wstringstream line;
            line << L"MinHook initialization failed: " << static_cast<int>(initStatus);
            AppendLog(line.str());
            return false;
        }

        const MH_STATUS createStatus = MH_CreateHook(
            reinterpret_cast<void*>(kStoreShadowAddress),
            reinterpret_cast<void*>(&HookStoreShadowToBeRendered),
            reinterpret_cast<void**>(&g_originalStoreShadow));
        if (createStatus != MH_OK) {
            std::wstringstream line;
            line << L"Shadow intensity hook creation failed: " << static_cast<int>(createStatus);
            AppendLog(line.str());
            MH_Uninitialize();
            return false;
        }

        const MH_STATUS enableStatus = MH_EnableHook(reinterpret_cast<void*>(kStoreShadowAddress));
        if (enableStatus != MH_OK) {
            std::wstringstream line;
            line << L"Shadow intensity hook enable failed: " << static_cast<int>(enableStatus);
            AppendLog(line.str());
            MH_RemoveHook(reinterpret_cast<void*>(kStoreShadowAddress));
            MH_Uninitialize();
            g_originalStoreShadow = nullptr;
            return false;
        }

        g_shadowHookInstalled = true;
        AppendLog(L"Installed hook: GTA native CShadows::StoreShadowToBeRendered (0x00707390).");
        return true;
    }

    bool ReadGtaClock(unsigned char& hour, unsigned char& minute) {
        // Plugin-SDK CClock addresses for GTA San Andreas 1.0 US:
        // ms_nGameClockMinutes = 0xB70152, ms_nGameClockHours = 0xB70153.
        constexpr uintptr_t kClockMinuteAddress = 0x00B70152;
        constexpr uintptr_t kClockHourAddress = 0x00B70153;

        MEMORY_BASIC_INFORMATION info{};
        if (VirtualQuery(reinterpret_cast<const void*>(kClockHourAddress),
                         &info, sizeof(info)) != sizeof(info)) return false;
        if (info.State != MEM_COMMIT || (info.Protect & (PAGE_NOACCESS | PAGE_GUARD)))
            return false;

        const auto* minutePtr = reinterpret_cast<volatile const unsigned char*>(kClockMinuteAddress);
        const auto* hourPtr = reinterpret_cast<volatile const unsigned char*>(kClockHourAddress);
        const unsigned char h = *hourPtr;
        const unsigned char m = *minutePtr;
        if (h > 23 || m > 59) return false;
        hour = h;
        minute = m;
        return true;
    }

    DWORD WINAPI Initialize(LPVOID) {
        SYSTEMTIME now{};
        GetLocalTime(&now);
        std::wstringstream header;
        header << L"StencilShadows loaded at " << std::setfill(L'0')
               << std::setw(2) << now.wHour << L":"
               << std::setw(2) << now.wMinute << L":"
               << std::setw(2) << now.wSecond;
        AppendLog(header.str());

        const ShadowConfig cfg = LoadConfig();
        LogConfig(cfg);
        AppendLog(L"Milestone: INI parsing and time-fade curve implemented.");

        if (!cfg.enabled) {
            AppendLog(L"Plugin disabled in INI; live clock polling skipped.");
            return 0;
        }

        int lastLoggedMinute = -1;
        bool clockWarningLogged = false;
        bool hookAttempted = false;
        while (InterlockedCompareExchange(&g_running, 1, 1) != 0) {
            unsigned char hour = 0, minute = 0;
            if (ReadGtaClock(hour, minute)) {
                clockWarningLogged = false;
                if (!hookAttempted) {
                    hookAttempted = true;
                    InstallShadowIntensityHook();
                }
                const float fractionalHour = static_cast<float>(hour) +
                                             static_cast<float>(minute) / 60.0f;
                const float strength = CalculateTimeStrength(cfg, fractionalHour);
                LONG bits = 0;
                static_assert(sizeof(bits) == sizeof(strength), "32-bit float expected");
                std::memcpy(&bits, &strength, sizeof(bits));
                InterlockedExchange(&g_strengthBits, bits);

                const int totalMinutes = static_cast<int>(hour) * 60 + minute;
                if (totalMinutes != lastLoggedMinute) {
                    std::wstringstream line;
                    line << L"GTA clock " << std::setfill(L'0')
                         << std::setw(2) << static_cast<int>(hour) << L":"
                         << std::setw(2) << static_cast<int>(minute)
                         << L" | shadow strength=" << std::fixed
                         << std::setprecision(3) << strength;
                    AppendLog(line.str());
                    lastLoggedMinute = totalMinutes;
                }
            } else if (!clockWarningLogged) {
                AppendLog(L"WARNING: GTA 1.0 US clock address is not readable yet; retrying.");
                clockWarningLogged = true;
            }
            Sleep(static_cast<DWORD>(cfg.updateIntervalMs));
        }
        if (g_shadowHookInstalled) {
            MH_DisableHook(reinterpret_cast<void*>(0x00707390));
            MH_Uninitialize();
            g_shadowHookInstalled = false;
            g_originalStoreShadow = nullptr;
        }
        return 0;
    }
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_module = module;
        InterlockedExchange(&g_running, 1);
        DisableThreadLibraryCalls(module);
        HANDLE thread = CreateThread(nullptr, 0, Initialize, nullptr, 0, nullptr);
        if (thread) CloseHandle(thread);
    } else if (reason == DLL_PROCESS_DETACH) {
        InterlockedExchange(&g_running, 0);
    }
    return TRUE;
}
