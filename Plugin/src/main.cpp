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
        auto address = reinterpret_cast<uintptr_t>(Assembly::search_pattern<"C4 E1 FA 2A C7 C5 F2 59 F0">());
        if (address)
        {
            INFO("PatchZeroWeight: Found hook address: {:x}", address);
            INFO("PatchZeroWeight: Settings->FractionOfWeight: {}", Settings::GetSingleton()->GetFractionOfWeight());

            Prolog prolog{};
            prolog.ready();
            Epilog epilog{};
            epilog.ready();
            
            const auto caveHookHandle = AddCaveHook(
                address, 
                { 0, 5 }, 
                FUNC_INFO(Hook_GetFractionOfWeight), 
                &prolog,
                &epilog,
                HookFlag::kRestoreBeforeProlog);
            caveHookHandle->Enable();
            INFO("PatchZeroWeight: hook applied");
        }
        else
        {
            ERROR("PatchZeroWeight: Couldn't find the address to hook");
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
    MessageBoxW(NULL, L"Loaded. You can attach the debugger now", L"SF-Zero-Weight Plugin", NULL);
#endif

    SFSE::Init(a_sfse, false);

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

    // A little workaround, until SFSE implements "version independence" that would be set by UsesSigScanning or UsesAddressLibrary
    data.CompatibleVersions({
        SFSE::RUNTIME_SF_1_7_29,
        SFSE::RUNTIME_LATEST,
        REL::Version(SFSE::RUNTIME_LATEST[0], SFSE::RUNTIME_LATEST[1], SFSE::RUNTIME_LATEST[2], SFSE::RUNTIME_LATEST[3] + 1),
        REL::Version(SFSE::RUNTIME_LATEST[0], SFSE::RUNTIME_LATEST[1], SFSE::RUNTIME_LATEST[2], SFSE::RUNTIME_LATEST[3] + 2),
        REL::Version(SFSE::RUNTIME_LATEST[0], SFSE::RUNTIME_LATEST[1], SFSE::RUNTIME_LATEST[2], SFSE::RUNTIME_LATEST[3] + 3),
        REL::Version(SFSE::RUNTIME_LATEST[0], SFSE::RUNTIME_LATEST[1], SFSE::RUNTIME_LATEST[2], SFSE::RUNTIME_LATEST[3] + 4),
        REL::Version(SFSE::RUNTIME_LATEST[0], SFSE::RUNTIME_LATEST[1], SFSE::RUNTIME_LATEST[2], SFSE::RUNTIME_LATEST[3] + 5),
        REL::Version(SFSE::RUNTIME_LATEST[0], SFSE::RUNTIME_LATEST[1], SFSE::RUNTIME_LATEST[2], SFSE::RUNTIME_LATEST[3] + 6),
        REL::Version(SFSE::RUNTIME_LATEST[0], SFSE::RUNTIME_LATEST[1], SFSE::RUNTIME_LATEST[2], SFSE::RUNTIME_LATEST[3] + 7),
        REL::Version(SFSE::RUNTIME_LATEST[0], SFSE::RUNTIME_LATEST[1], SFSE::RUNTIME_LATEST[2], SFSE::RUNTIME_LATEST[3] + 8),
        REL::Version(SFSE::RUNTIME_LATEST[0], SFSE::RUNTIME_LATEST[1], SFSE::RUNTIME_LATEST[2], SFSE::RUNTIME_LATEST[3] + 9),
        REL::Version(SFSE::RUNTIME_LATEST[0], SFSE::RUNTIME_LATEST[1], SFSE::RUNTIME_LATEST[2], SFSE::RUNTIME_LATEST[3] + 10),
        REL::Version(SFSE::RUNTIME_LATEST[0], SFSE::RUNTIME_LATEST[1], SFSE::RUNTIME_LATEST[2], SFSE::RUNTIME_LATEST[3] + 11),
        REL::Version(SFSE::RUNTIME_LATEST[0], SFSE::RUNTIME_LATEST[1], SFSE::RUNTIME_LATEST[2], SFSE::RUNTIME_LATEST[3] + 12),
        REL::Version(SFSE::RUNTIME_LATEST[0], SFSE::RUNTIME_LATEST[1], SFSE::RUNTIME_LATEST[2], SFSE::RUNTIME_LATEST[3] + 13),
        REL::Version(SFSE::RUNTIME_LATEST[0], SFSE::RUNTIME_LATEST[1], SFSE::RUNTIME_LATEST[2], SFSE::RUNTIME_LATEST[3] + 14)
    });

    return data;
}();