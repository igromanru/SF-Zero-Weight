#include "DKUtil/Hook.hpp"
#include "DKUtil/Config.hpp"

using namespace DKUtil::Alias;
using namespace DKUtil::Hook;

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
    data.CompatibleVersions({
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
    });

    return data;
}();

namespace
{
    class Settings : public dku::model::Singleton<Settings>
    {
    private:
        TomlConfig mainConfig = COMPILE_PROXY(Plugin::SETTINGS_NAME);

        Double fractionOfWeight{"fractionOfWeight"};
    public:
        float getFractionOfWeight()
        {
            return static_cast<float>(*fractionOfWeight);
        }

        void Load() noexcept
        {
            static std::once_flag bound;
            std::call_once(bound, [&]() { mainConfig.Bind<0.0, 10.0>(fractionOfWeight, 0.0); });

            mainConfig.Load();
        }

        void Save() noexcept
        {
            mainConfig.GenerateIfMissing();
            mainConfig.Write();
        }
    };

    void __cdecl Placeholder() {}

    void PatchZeroWeight()
    {
        auto address = reinterpret_cast<uintptr_t>(Assembly::search_pattern<"C4 E1 FA 2A C7 C5 F2 59 F0">());
        if (address > 0)
        {
            INFO("PatchZeroWeight: Found hook address: {:x}", address);
            auto fractionOfWeight = Settings::GetSingleton()->getFractionOfWeight();
            INFO("PatchZeroWeight: Settings->FractionOfWeight: {}", fractionOfWeight);
            auto* fractionOfWeightPtr = reinterpret_cast<uint8_t*>(&fractionOfWeight);
            DEBUG("PatchZeroWeight: FractionOfWeight as bytes: 0x{:x}, 0x{:x}, 0x{:x}, 0x{:x}", fractionOfWeightPtr[0], 
                fractionOfWeightPtr[1], fractionOfWeightPtr[2], fractionOfWeightPtr[3]);

            const OpCode caveCode[] 
            {
                0x48, 0xBF, fractionOfWeightPtr[0], fractionOfWeightPtr[1], fractionOfWeightPtr[2], fractionOfWeightPtr[3], // mov rdi, (config float value as big edian)
                0x00, 0x00, 0x00, 0x00,
                0x66, 0x48, 0x0F, 0x6E, 0xFF, // movq xmm7,rdi
                0xC5, 0xFA, 0x59, 0xC7 // vmulss xmm0,xmm0,xmm7
            };
            const auto caveHook = AddCaveHook(address, std::make_pair(0, 5), FUNC_INFO(Placeholder), {&caveCode, sizeof(caveCode)}, std::make_pair(nullptr, 0), HookFlag::kRestoreBeforeProlog);
            caveHook->Enable();
            INFO("PatchZeroWeight: hook applied", fractionOfWeight);
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
    SFSE::AllocTrampoline(1 << 10);

    SFSE::GetMessagingInterface()->RegisterListener(MessageCallback);

    return true;
}
