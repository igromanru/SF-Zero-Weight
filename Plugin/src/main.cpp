#include "PCH.h"

namespace
{
    union FloatInt
    {
        float floatValue;
        int intValue;
    };

    struct CodeCave : Xbyak::CodeGenerator
    {
        CodeCave(FloatInt fractionOfWeight)
        {
            // Original code
            vcvtsi2ss(xmm1, xmm1, rax);

            // Save previous values
            push(rdi);
            sub(rsp, 0x10);
            movdqu(ptr[rsp], xmm8);

            // Calculate new weight
            mov(edi, fractionOfWeight.intValue);
            movd(xmm8, edi);
            vmulss(xmm1, xmm1, xmm8);

            // Restore values
            movdqu(xmm8, ptr[rsp]);
            add(rsp, 0x10);
            pop(rdi);
        }
    };

    bool PatchZeroWeight()
    {
        const auto hookAddress = reinterpret_cast<uintptr_t>(Assembly::search_pattern<"C4 E1 F2 2A C8 C5 FA 59 C1 EB">());
        if (hookAddress)
        {
            const FloatInt fractionOfWeight = { Settings::GetSingleton()->GetFractionOfWeight() };
            INFO("Found hook address: {:x}", hookAddress);
            INFO("Settings->FractionOfWeight: {}", fractionOfWeight.floatValue);

            CodeCave cave{ fractionOfWeight };
            cave.ready();

            const auto patchHandle = AddASMPatch(hookAddress, { 0, 5 }, &cave);
            patchHandle->Enable();
            INFO("Hook applied");
        }
        else
        {
            ERROR("Couldn't find the address to hook");
        }
        return false;
    }
} // namespace

DWORD WINAPI Thread(LPVOID param)
{
    Settings::GetSingleton()->Load();
    return PatchZeroWeight();
}

extern "C" __declspec(dllexport) void InitializeASI()
{
#ifndef NDEBUG
    MessageBoxA(NULL, "Loaded. You can attach the debugger now", "SF LongerNames ASI Plugin", NULL);
#endif
    dku::Logger::Init(Plugin::NAME, std::to_string(Plugin::Version));
    INFO("Game: {}, base: {:x}", dku::Hook::GetProcessName(), Module::get().base());
    Trampoline::AllocTrampoline(256);

    CloseHandle(CreateThread(nullptr, 0, Thread, nullptr, 0, nullptr));
}


BOOL APIENTRY DllMain(HMODULE a_hModule, DWORD a_dwReason, LPVOID a_lpReserved)
{
    if (a_dwReason == DLL_PROCESS_ATTACH)
    {/*
#ifndef NDEBUG
        MessageBoxA(NULL, "Loaded. You can attach the debugger now", "SF-Zero-Weight ASI Plugin", NULL);
#endif
        dku::Logger::Init(Plugin::NAME, std::to_string(Plugin::Version));
        INFO("Game: {}", dku::Hook::GetProcessName());
        Trampoline::AllocTrampoline(256);

        CloseHandle(CreateThread(nullptr, 0, Thread, nullptr, 0, nullptr));*/
    }

    return TRUE;
}