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
            vcvtsi2ss(xmm0, xmm0, rdi);

            // Save previous values
            push(rdi);
            sub(rsp, 0x10);
            movdqu(ptr[rsp], xmm1);

            // Calculate new weight
            mov(eax, fractionOfWeight.intValue);
            movd(xmm1, eax);
            vmulss(xmm0, xmm0, xmm1);

            // Restore values
            movdqu(xmm1, ptr[rsp]);
            add(rsp, 0x10);
            pop(rdi);
        }
    };


    void PatchZeroWeight()
    {
        const auto hookAddress = reinterpret_cast<uintptr_t>(Assembly::search_pattern<"C4 E1 F2 2A C8 C5 FA 59 C1 EB">());
        if (hookAddress)
        {
            const FloatInt fractionOfWeight = { Settings::GetSingleton()->GetFractionOfWeight() };
            INFO("Found hook address: {:x}. Game base: {:x}", hookAddress, Module::get().base());
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
    }

    void MessageCallback(SFSE::MessagingInterface::Message *a_msg) noexcept
    {
        switch (a_msg->type)
        {
        case SFSE::MessagingInterface::kPostLoad:
            Settings::GetSingleton()->Load();
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
    MessageBoxA(NULL, "Loaded. You can attach the debugger now", Plugin::NAME.data(), NULL);
#endif

    Init(a_sfse, false);

    DKUtil::Logger::Init(Plugin::NAME, std::to_string(Plugin::Version));

    INFO("{} v{} loaded", Plugin::NAME, Plugin::Version);

    SFSE::AllocTrampoline(128);

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
        SFSE::RUNTIME_LATEST
    });

    return data;
}();