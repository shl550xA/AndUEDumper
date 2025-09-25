#pragma once

#include "../UEGameProfile.hpp"
using namespace UEMemory;

class PUBGProfile : public IGameProfile
{
public:
    PUBGProfile() = default;

    bool ArchSupprted() const override
    {
        auto e_machine = GetUnrealELF().header().e_machine;
        return e_machine == EM_AARCH64;
    }

    std::string GetAppName() const override
    {
        return "PUBG";
    }

    std::vector<std::string> GetAppIDs() const override
    {
        return {
            "com.tencent.ig",
            "com.rekoo.pubgm",
            "com.pubg.imobile",
            "com.pubg.krmobile",
            "com.vng.pubgmobile",
        };
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
        auto map_type = isEmulator() ? PATTERN_MAP_TYPE::ANY_R : PATTERN_MAP_TYPE::ANY_X;
        return Arm64::Decode_ADRP_ADD(findIdaPattern(map_type, "12 40 b9 ? 3e 40 b9 ? ? ? 6b ? ? ? 54 ? ? ? ? ? ? ? 91", 15));
    }

    uintptr_t GetNameToStringPtr() const override
    {
        auto map_type = isEmulator() ? PATTERN_MAP_TYPE::ANY_R : PATTERN_MAP_TYPE::ANY_X;
        return findIdaPattern(map_type, "f4 03 00 aa f3 03 08 aa ? ? ? 34 ? ? ? a9 e1 03 00 91", -5 * 4);
    }

    UE_Offsets *GetOffsets() const override
    {
        static UE_Offsets offsets = UE_DefaultOffsets::UE4_18_19(isUsingCasePreservingName());
        return &offsets;
    }
};