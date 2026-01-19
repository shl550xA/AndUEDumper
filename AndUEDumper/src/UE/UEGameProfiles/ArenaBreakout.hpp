#pragma once

#include "../UEGameProfile.hpp"
using namespace UEMemory;

class ArenaBreakoutProfile : public IGameProfile
{
public:
    ArenaBreakoutProfile() = default;

    bool ArchSupprted() const override
    {
        auto e_machine = GetUnrealELF().header().e_machine;
        // only arm64
        return e_machine == EM_AARCH64;
    }

    std::string GetAppName() const override
    {
        return "Arena Breakout";
    }

    std::vector<std::string> GetAppIDs() const override
    {
        return {"com.proximabeta.mf.uamo"};
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
        auto match = findIdaPattern(map_type, "? ? ? aa ? ? ? f9 ? ? ? aa ? ? ? f8 ? ? ? b9 ? ? ? 35 ? ? ? ? ? ? ? 39", 0);
        if (Arm64::DecodeADRL(match + 4 * 6) != 0)
        {
            return match - 4 * 9;
        }
        return 0;
    }

    UE_Offsets *GetOffsets() const override
    {
        static UE_Offsets offsets = UE_DefaultOffsets::UE4_25_27(isUsingCasePreservingName());

        static bool once = false;
        if (!once)
        {
            once = true;

            offsets.UObject.ObjectFlags = sizeof(void *);
            offsets.UObject.InternalIndex = offsets.UObject.ObjectFlags + sizeof(int32_t);
            offsets.UObject.ClassPrivate = offsets.UObject.InternalIndex + sizeof(int32_t);
            offsets.UObject.NamePrivate = offsets.UObject.ClassPrivate + sizeof(void *);
            offsets.UObject.OuterPrivate = GetPtrAlignedOf(offsets.UObject.NamePrivate + offsets.FName.Size);

            offsets.UField.Next = offsets.UObject.OuterPrivate + sizeof(void *) * 2;  // sizeof(UObject)

            offsets.UEnum.Names = offsets.UField.Next + (sizeof(void *) * 2) + (sizeof(int32_t) * 2);  // usually at sizeof(UField) + sizeof(FString)

            offsets.UStruct.SuperStruct = offsets.UField.Next + (sizeof(void *) * 3);  // sizeof(UField) + sizeof(FStructBaseChain)
            offsets.UStruct.Children = offsets.UStruct.SuperStruct + sizeof(void *);   // UField*
            offsets.UStruct.PropertiesSize = offsets.UStruct.Children + sizeof(void *);

            offsets.UFunction.EFunctionFlags = offsets.UStruct.PropertiesSize + (sizeof(int32_t) * 2) + ((sizeof(void *) + sizeof(int32_t) * 2) * 2) + (sizeof(void *) * 4);
            offsets.UFunction.NumParams = offsets.UFunction.EFunctionFlags + sizeof(int32_t);
            offsets.UFunction.ParamSize = offsets.UFunction.NumParams + sizeof(int16_t);
            offsets.UFunction.Func = offsets.UFunction.EFunctionFlags + (sizeof(int32_t) * 4) + (sizeof(void *) * 3);

            offsets.UProperty.ArrayDim = offsets.UField.Next + sizeof(void *);  // sizeof(UField)
            offsets.UProperty.ElementSize = offsets.UProperty.ArrayDim + sizeof(int32_t);
            offsets.UProperty.PropertyFlags = GetPtrAlignedOf(offsets.UProperty.ElementSize + sizeof(int32_t));
            offsets.UProperty.Offset_Internal = offsets.UProperty.PropertyFlags + sizeof(int64_t) + sizeof(int32_t);
            offsets.UProperty.Size = GetPtrAlignedOf(offsets.UProperty.Offset_Internal + sizeof(int32_t) + offsets.FName.Size) + (sizeof(void *) * 4);  // sizeof(UProperty)

            offsets.UStruct.ChildProperties = offsets.UStruct.Children + sizeof(void *);  // FField*
            offsets.UStruct.PropertiesSize = offsets.UStruct.ChildProperties + sizeof(void *);

            offsets.UFunction.EFunctionFlags = offsets.UStruct.PropertiesSize + (sizeof(int32_t) * 2) + ((sizeof(void *) + sizeof(int32_t) * 2) * 2) + (sizeof(void *) * 6);
            offsets.UFunction.NumParams = offsets.UFunction.EFunctionFlags + sizeof(int32_t);
            offsets.UFunction.ParamSize = offsets.UFunction.NumParams + sizeof(int16_t);
            offsets.UFunction.Func = offsets.UFunction.EFunctionFlags + (sizeof(int32_t) * 4) + (sizeof(void *) * 3);

            offsets.FField.ClassPrivate = sizeof(void *);
            offsets.FField.Next = offsets.FField.ClassPrivate + (sizeof(void *) * 3);  // + sizeof(FFieldVariant);
            offsets.FField.NamePrivate = offsets.FField.Next + sizeof(void *);
            offsets.FField.FlagsPrivate = offsets.FField.NamePrivate + offsets.FName.Size;

            offsets.FProperty.ArrayDim = offsets.FField.FlagsPrivate + sizeof(int32_t);  // sizeof(UFField)
            offsets.FProperty.ElementSize = offsets.FProperty.ArrayDim + sizeof(int32_t);
            offsets.FProperty.PropertyFlags = GetPtrAlignedOf(offsets.FProperty.ElementSize + sizeof(int32_t));
            offsets.FProperty.Offset_Internal = offsets.FProperty.PropertyFlags + sizeof(int64_t) + sizeof(int32_t);
            offsets.FProperty.Size = GetPtrAlignedOf(offsets.FProperty.Offset_Internal + sizeof(int32_t) + offsets.FName.Size) + (sizeof(void *) * 4);  // sizeof(FProperty)

            offsets.UProperty.ArrayDim = 0;
            offsets.UProperty.ElementSize = 0;
            offsets.UProperty.PropertyFlags = 0;
            offsets.UProperty.Offset_Internal = 0;
            offsets.UProperty.Size = 0;
        }
        return &offsets;
    }
};
