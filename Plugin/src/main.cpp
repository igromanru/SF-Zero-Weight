#include "PCH.h"

namespace
{
    float Hook_GetFractionOfWeight() 
    {
        // maybe some calculations
        return Settings::GetSingleton()->GetFractionOfWeight();
    }

    struct Prolog : Xbyak::CodeGenerator
    {
        Prolog()
        {
            // save xmm1
            sub(rsp, 0x10);
            movdqu(ptr[rsp], xmm1);

            // save xmm0
            sub(rsp, 0x10);
            movdqu(ptr[rsp], xmm0);
        }
    };

    struct Epilog : Xbyak::CodeGenerator
    {
        Epilog()
        {
            // assign return value
            vmovaps(xmm1, xmm0);

            // restore xmm0
            movdqu(xmm0, ptr[rsp]);
            add(rsp, 0x10);

            // calc new value
            vmulss(xmm0, xmm0, xmm1);

            // restore xmm1
            movdqu(xmm1, ptr[rsp]);
            add(rsp, 0x10);
        }
    };

    bool PatchZeroWeight()
    {
        const auto hookAddress = reinterpret_cast<uintptr_t>(Assembly::search_pattern<"C4 E1 FA 2A C7 C5 F2 59 F0">());
        if (hookAddress)
        {
            INFO("Found hook address: {:x}. Game base: {:x}", hookAddress, Module::get().base());
            INFO("Settings->FractionOfWeight: {}", Settings::GetSingleton()->GetFractionOfWeight());

            Trampoline::AllocTrampoline(128);

            Prolog prolog{};
            prolog.ready();
            Epilog epilog{};
            epilog.ready();
            
            const auto caveHookHandle = AddCaveHook(
                hookAddress, 
                { 0, 5 }, 
                FUNC_INFO(Hook_GetFractionOfWeight), 
                &prolog,
                &epilog,
                HookFlag::kRestoreBeforeProlog);
            caveHookHandle->Enable();
            INFO("Hook applied");
            return true;
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


BOOL APIENTRY DllMain(HMODULE a_hModule, DWORD a_dwReason, LPVOID a_lpReserved)
{
    if (a_dwReason == DLL_PROCESS_ATTACH)
    {
#ifndef NDEBUG
        MessageBoxA(NULL, "Loaded. You can attach the debugger now", "SF-Zero-Weight ASI Plugin", NULL);
#endif
        dku::Logger::Init(Plugin::NAME, std::to_string(Plugin::Version));
        INFO("Game: {}", dku::Hook::GetProcessName());

        CloseHandle(CreateThread(nullptr, 0, Thread, nullptr, 0, nullptr));
    }

    return TRUE;
}