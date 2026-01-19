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
            "com.tencent.tmgp.pubgmhd",
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
        std::vector<std::pair<std::string, int>> idaPatterns = {
            {"? ? ? aa ? ? ? 34 ? ? ? 91 ? ? ? aa ? ? ? A9 ? ? ? ? ? ? ? a9", -7 * 4},
            {"f4 03 00 aa f3 03 08 aa ? ? ? 34 ? ? ? a9 e1 03 00 91", -5 * 4},
        };

        PATTERN_MAP_TYPE map_type = isEmulator() ? PATTERN_MAP_TYPE::ANY_R : PATTERN_MAP_TYPE::ANY_X;

        for (const auto &it : idaPatterns)
        {
            std::string ida_pattern = it.first;
            const int step = it.second;

            if (uintptr_t x = findIdaPattern(map_type, ida_pattern, step))
            {
                return x;
            }
        }

        return 0;
    }

    UE_Offsets *GetOffsets() const override
    {
        static UE_Offsets offsets = UE_DefaultOffsets::UE4_18_19(isUsingCasePreservingName());

        static bool once = false;
        if (!once)
        {
            once = true;

            extern char const *__progname;

            if (std::string{__progname} == "com.tencent.tmgp.pubgmhd")
            {
                offsets.FUObjectArray.ObjObjects = 0xC8;
                offsets.TUObjectArray.NumElements = 0x38;
            }
        }

        return &offsets;
    }
};