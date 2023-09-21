#include "DKUtil/Hook.hpp"
#include "FindPattern.hpp"

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
        const auto address = FindPattern(GetModuleHandleW(nullptr), reinterpret_cast<const unsigned char *>("\xC4\xE1\xFA\x2A\xC7\xC5\xF2\x59\xF0"), "xxxxxxxxx");
        if (address > 0)
        {
            INFO("PatchZeroWeight: Found patch address: {0:x}", address);
            constexpr char patchBytesArray[] = "\xC5\xF8\x57\xC0\x90"; // vxorps xmm0,xmm0,xmm0 nop
            constexpr int patchBytesArraySize = std::size(patchBytesArray) - 1;
            DWORD protectBackup;
            if (VirtualProtect(reinterpret_cast<void *>(address), patchBytesArraySize, PAGE_EXECUTE_READWRITE, &protectBackup))
            {
                DEBUG("PatchZeroWeight: Patch size: {0:x}", patchBytesArraySize);
                for (int i = 0; i < patchBytesArraySize; i++)
                {
                    reinterpret_cast<char *>(address)[i] = patchBytesArray[i];
                }
                if (VirtualProtect(reinterpret_cast<void *>(address), patchBytesArraySize, protectBackup, &protectBackup))
                {
                    INFO("PatchZeroWeight: Patch applied");
                }
            }
            else
            {
                ERROR("PatchZeroWeight: Couldn't change memory protection");
            }
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
