#pragma once

#include "../UEGameProfile.hpp"
using namespace UEMemory;

class ValorantProfile : public IGameProfile
{
public:
    ValorantProfile() = default;

    bool ArchSupprted() const override
    {
        auto e_machine = GetUnrealELF().header().e_machine;
        // only arm64
        return e_machine == EM_AARCH64;
    }

    std::string GetAppName() const override
    {
        return "Valorant";
    }

    std::vector<std::string> GetAppIDs() const override
    {
        return {"com.tencent.tmgp.codev"};
    }

    bool isUsingCasePreservingName() const override
    {
        return false;
    }

    bool isUsingOutlineNumberName() const override
    {
        return false;
    }

    uintptr_t GetGUObjectArrayPtr() const override
    {
        return GetUEVars()->GetBaseAddress() + 0xA7A4670;
    }

    uintptr_t GetNameToStringPtr() const override
    {
        return GetUEVars()->GetBaseAddress() + 0x7FA9014;
    }

    UE_Offsets *GetOffsets() const override
    {
        static UE_Offsets offsets = UE_DefaultOffsets::UE4_25_27(isUsingCasePreservingName());

        static bool once = false;
        if (!once)
        {
            once = true;

            offsets.UFunction.Func = 0xE0;
        }

        return &offsets;
    }
};
