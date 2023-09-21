#include "DKUtil/Hook.hpp"

DLLEXPORT constinit auto SFSEPlugin_Version = []() noexcept
{
    SFSE::PluginVersionData data{};

    data.PluginVersion(Plugin::Version);
    data.PluginName(Plugin::NAME);
    data.AuthorName(Plugin::AUTHOR);
    data.UsesSigScanning(true);
    // data.UsesAddressLibrary(true);
    data.HasNoStructUse(true);
    // data.IsLayoutDependent(true);
    data.CompatibleVersions({SFSE::RUNTIME_LATEST});

    return data;
}();

namespace
{
    void PatchZeroWeight()
    {
        auto* address = dku::Hook::Assembly::search_pattern<"C4 E1 FA 2A C7 C5 F2 59 F0">();
        if (address)
        {
            INFO("PatchZeroWeight: Found patch address: {0:x}", reinterpret_cast<uintptr_t>(address));
            constexpr DKUtil::Alias::OpCode patchAsm[]{
                0xC5, 0xF8, 0x57, 0xC0, // vxorps xmm0,xmm0,xmm0
                0x90, // nop
            };
            constexpr int patchSize = std::size(patchAsm);
            const auto patchRef = dku::Hook::AddASMPatch(reinterpret_cast<uintptr_t>(address), std::make_pair(0, patchSize), {&patchAsm, sizeof(patchAsm)});
            patchRef->Enable();
        }
        else
        {
            ERROR("PatchZeroWeight: Couldn't find the address to patch");
        }
    }

    void MessageCallback(SFSE::MessagingInterface::Message *a_msg) noexcept
    {
        switch (a_msg->type)
        {
        case SFSE::MessagingInterface::kPostLoad:
            PatchZeroWeight();
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
    SFSE::AllocTrampoline(1 << 10);

    SFSE::GetMessagingInterface()->RegisterListener(MessageCallback);

    return true;
}
