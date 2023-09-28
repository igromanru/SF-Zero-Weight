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

    void PatchZeroWeight()
    {
        const auto hookAddress = reinterpret_cast<uintptr_t>(Assembly::search_pattern<"C4 E1 FA 2A C7 C5 F2 59 F0">());
        if (hookAddress)
        {
            INFO("Found hook address: {:x}. Game base: {:x}", hookAddress, Module::get().base());
            INFO("Settings->FractionOfWeight: {}", Settings::GetSingleton()->GetFractionOfWeight());

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
        }
        else
        {
            ERROR("Couldn't find the address to hook");
        }
    }

    void MessageCallback(SFSE::MessagingInterface::Message *a_msg) noexcept
    {
        switch (a_msg->type)
        {
        case SFSE::MessagingInterface::kPostLoad:
            Settings::GetSingleton()->Load();
            PatchZeroWeight();
            // Settings::GetSingleton()->Save();
            break;
        default:
            break;
        }
    }
} // namespace

/**
// for preload plugins
void SFSEPlugin_Preload(SFSE::LoadInterface* a_sfse);
/**/

DLLEXPORT bool SFSEAPI SFSEPlugin_Load(const SFSE::LoadInterface *a_sfse)
{
#ifndef NDEBUG
    MessageBoxA(NULL, "Loaded. You can attach the debugger now", "SF-Zero-Weight Plugin", NULL);
#endif

    Init(a_sfse, false);

    DKUtil::Logger::Init(Plugin::NAME, std::to_string(Plugin::Version));

    INFO("{} v{} loaded", Plugin::NAME, Plugin::Version);

    // do stuff
    SFSE::AllocTrampoline(1 << 7);

    SFSE::GetMessagingInterface()->RegisterListener(MessageCallback);

    return true;
}

DLLEXPORT constexpr auto SFSEPlugin_Version = []() noexcept
{
    SFSE::PluginVersionData data{};

    data.PluginVersion(Plugin::Version);
    data.PluginName(Plugin::NAME);
    data.AuthorName(Plugin::AUTHOR);
    data.UsesSigScanning(true);
    // data.UsesAddressLibrary(false);
    data.HasNoStructUse(true);
    data.IsLayoutDependent(false);
    data.CompatibleVersions({
        SFSE::RUNTIME_SF_1_6_35,
        SFSE::RUNTIME_SF_1_7_23,
        SFSE::RUNTIME_SF_1_7_29,
        SFSE::RUNTIME_LATEST
    });

    return data;
}();