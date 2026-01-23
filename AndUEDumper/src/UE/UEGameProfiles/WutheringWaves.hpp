#pragma once

#include "../UEGameProfile.hpp"
using namespace UEMemory;

class WutheringWavesProfile : public IGameProfile
{
public:
    WutheringWavesProfile() = default;

    bool ArchSupprted() const override
    {
        auto e_machine = GetUnrealELF().header().e_machine;
        return e_machine == EM_AARCH64;
    }

    std::string GetAppName() const override
    {
        return "Wuthering Waves";
    }

    std::vector<std::string> GetAppIDs() const override
    {
        return {"com.kurogame.wutheringwaves.global"};
    }

    bool isUsingCasePreservingName() const override
    {
        return true;
    }

    bool isUsingOutlineNumberName() const override
    {
        return false;
    }

    uintptr_t GetGUObjectArrayPtr() const override
    {
        uintptr_t guobjectarray = GetUnrealELF().findSymbol("GUObjectArray");
        if (guobjectarray != 0)
            return guobjectarray;

        std::vector<std::pair<std::string, int>> idaPatterns = {
            {"91 E1 03 ? AA E0 03 08 AA E2 03 1F 2A", -7},
            {"B4 21 0C 40 B9 ? ? ? ? ? ? ? 91", 5},
            {"9F E5 00 ? 00 E3 FF ? 40 E3 ? ? A0 E1", -2},
            {"96 df 02 17 ? ? ? ? 54 ? ? ? ? ? ? ? 91 e1 03 13 aa", 9},
            {"f4 03 01 2a ? 00 00 34 ? ? ? ? ? ? ? ? ? ? 00 54 ? 00 00 14 ? ? ? ? ? ? ? 91", 0x18},
            {"69 3e 40 b9 1f 01 09 6b ? ? ? 54 e1 03 13 aa ? ? ? ? f4 4f ? a9 ? ? ? ? ? ? ? 91", 0x18},
        };

        PATTERN_MAP_TYPE map_type = isEmulator() ? PATTERN_MAP_TYPE::ANY_R : PATTERN_MAP_TYPE::ANY_X;

        for (const auto &it : idaPatterns)
        {
            std::string ida_pattern = it.first;
            const int step = it.second;

            uintptr_t adrl = Arm64::DecodeADRL(findIdaPattern(map_type, ida_pattern, step));
            if (adrl != 0) return adrl;
        }

        return 0;
    }

    uintptr_t GetNameToStringPtr() const override
    {
        auto map_type = isEmulator() ? PATTERN_MAP_TYPE::ANY_R : PATTERN_MAP_TYPE::ANY_X;
        return findIdaPattern(map_type, "f4 03 00 aa f3 03 08 aa ? ? ? f8 09 08 40 b9 ? ? ? 34 ? ? ? 91 ? ? ? aa ? ? ? a9", -8 * 4);
    }

    UE_Offsets *GetOffsets() const override
    {
        static UE_Offsets offsets = UE_DefaultOffsets::UE4_25_27(isUsingCasePreservingName());
        return &offsets;
    }
};
