#pragma once

#include "DKUtil/Config.hpp"

using namespace DKUtil::Alias;

class Settings : public dku::model::Singleton<Settings>
{
private:
    TomlConfig mainConfig = COMPILE_PROXY(Plugin::SETTINGS_NAME);

    Double fractionOfWeight{"fractionOfWeight"};
public:
    float GetFractionOfWeight()
    {
        return static_cast<float>(*fractionOfWeight);
    }

    void Load() noexcept
    {
        static std::once_flag bound;
        std::call_once(bound, [&]() { mainConfig.Bind<0.0, 10.0>(fractionOfWeight, 0.0); });

        mainConfig.Load();
    }
};
