#include "UEGameProfile.hpp"

#include "UEMemory.hpp"
#include "UEWrappers.hpp"

using namespace UEMemory;

UEVarsInitStatus IGameProfile::InitUEVars()
{
    bool is32Bit = KittyMemoryEx::getMaps(kMgr.processID(), EProcMapFilter::EndWith, "/linker64").empty();
    if (is32Bit)
    {
        if (sizeof(void *) != 4)
        {
            LOGE("Dumper is 64bit while target process is 32bit. Please use the correct architecture.");
            return UEVarsInitStatus::ERROR_ARCH_MISMATCH;
        }
    }
    else
    {
        if (sizeof(void *) != 8)
        {
            LOGE("Dumper is 32bit while target process is 64bit. Please use the correct architecture.");
            return UEVarsInitStatus::ERROR_ARCH_MISMATCH;
        }
    }

    auto ue_elf = GetUnrealELF();
    if (!ue_elf.isValid())
    {
        LOGE("Couldn't find a valid UE ELF in target process maps.");
        return UEVarsInitStatus::ERROR_LIB_NOT_FOUND;
    }

    if (!ArchSupprted())
    {
        if (GetUnrealELF().header().e_machine > 0 && !ue_elf.isFixedBySoInfo())
        {
            LOGE("Architecture ( 0x%x ) is not supported for this game.", ue_elf.header().e_machine);
            return UEVarsInitStatus::ARCH_NOT_SUPPORTED;
        }
        else
        {
            LOGW("UE ELF Header might have been removed or modified!");
        }
    }

    LOGI("Library: %s", ue_elf.realPath().c_str());
    LOGI("BaseAddress: %p", (void *)ue_elf.base());
    LOGI("==========================");

    kPtrValidator.setPID(kMgr.processID());
    kPtrValidator.setUseCache(true);
    kPtrValidator.refreshRegionCache();
    if (kPtrValidator.cachedRegions().empty())
        return UEVarsInitStatus::ERROR_INIT_PTR_VALIDATOR;

    _UEVars.BaseAddress = ue_elf.base();

    UE_Offsets *pOffsets = GetOffsets();
    if (!pOffsets)
        return UEVarsInitStatus::ERROR_INIT_OFFSETS;

    _UEVars.Offsets = pOffsets;

    _UEVars.GUObjectsArrayPtr = GetGUObjectArrayPtr();
    if (!kPtrValidator.isPtrReadable(_UEVars.GUObjectsArrayPtr))
        return UEVarsInitStatus::ERROR_INIT_GUOBJECTARRAY;

    _UEVars.NameToStringPtr = GetNameToStringPtr();
    if (!kPtrValidator.isPtrExecutable(_UEVars.NameToStringPtr))
        return UEVarsInitStatus::ERROR_INIT_GNAMES;

    _UEVars.ObjObjectsPtr = _UEVars.GUObjectsArrayPtr + pOffsets->FUObjectArray.ObjObjects;

    if (!vm_rpm_ptr((void *)(_UEVars.ObjObjectsPtr + pOffsets->TUObjectArray.Objects),
                    &_UEVars.ObjObjects_Objects, sizeof(uintptr_t)))
        return UEVarsInitStatus::ERROR_INIT_OBJOBJECTS;

    UEWrappers::Init(GetUEVars());

    return UEVarsInitStatus::SUCCESS;
}

std::vector<std::string> IGameProfile::GetUESoNames() const
{
    return {"libUE4.so",
            "libUnreal.so"};
}

ElfScanner IGameProfile::GetUnrealELF() const
{
    static const std::vector<std::string> cUELibNames = GetUESoNames();

    thread_local static ElfScanner ue_elf{};
    if (ue_elf.isValid())
        return ue_elf;

    // find via linker or nativebridge solist
    // some games like farlight remove ELF header from lib
    for (const auto &lib : cUELibNames)
    {
        auto nativeSo = kMgr.linkerScanner.findSoInfo(lib);
        if (nativeSo.ptr)
        {
            ue_elf = kMgr.elfScanner.createWithSoInfo(nativeSo);
            if (ue_elf.isValid())
                return ue_elf;
        }

        auto emulatedSo = kMgr.nbScanner.findSoInfo(lib);
        if (emulatedSo.ptr)
        {
            ue_elf = kMgr.elfScanner.createWithSoInfo(emulatedSo);
            if (ue_elf.isValid())
                return ue_elf;
        }
    }

    for (const auto &lib : cUELibNames)
    {
        ue_elf = kMgr.elfScanner.findElf(lib, EScanElfType::Any, EScanElfFilter::App);
        if (ue_elf.isValid())
            return ue_elf;
    }

    return ue_elf;
}

bool IGameProfile::isEmulator() const
{
    const auto elf = GetUnrealELF();
    return (elf.isValid() && kMgr.elfScanner.isElfEmulated(elf)) || kMgr.nbScanner.isValid();
}

uintptr_t IGameProfile::findIdaPattern(PATTERN_MAP_TYPE map_type,
                                       const std::string &pattern,
                                       const int step,
                                       uint32_t skip_result) const
{
    ElfScanner ue_elf = GetUnrealELF();
    std::vector<KittyMemoryEx::ProcMap> search_segments;
    bool hasBSS = ue_elf.bssSegments().size() > 0;

    if (map_type == PATTERN_MAP_TYPE::BSS)
    {
        if (!hasBSS)
            return 0;

        for (auto &it : ue_elf.bssSegments())
            search_segments.push_back(it);
    }
    else
    {
        for (auto &it : ue_elf.segments())
        {
            if (!it.readable || !it.is_private)
                continue;

            if (map_type == PATTERN_MAP_TYPE::ANY_X && !it.executable)
                continue;
            else if (map_type == PATTERN_MAP_TYPE::ANY_W && !it.writeable)
                continue;

            search_segments.push_back(it);
        }
    }

    LOGD("search_segments count = %p", (void *)search_segments.size());

    uintptr_t insn_address = 0;

    for (auto &it : search_segments)
    {
        if (skip_result > 0)
        {
            auto adr_list = kMgr.memScanner.findIdaPatternAll(it.startAddress,
                                                              it.endAddress, pattern);
            if (adr_list.size() > skip_result)
            {
                insn_address = adr_list[skip_result];
            }
        }
        else
        {
            insn_address = kMgr.memScanner.findIdaPatternFirst(
                it.startAddress, it.endAddress, pattern);
        }
        if (insn_address)
            break;
    }
    return (insn_address ? (insn_address + step) : 0);
}

std::vector<std::string> IGameProfile::GetExcludedObjects() const
{
    // full name
    /*return {
        "ScriptStruct CoreUObject.Vector",
        "ScriptStruct CoreUObject.Vector2D"
    };*/
    return {};
}

std::string IGameProfile::GetUserTypesHeader() const
{
    return R"(#pragma once
    
#include <cstdio>
#include <string>
#include <cstdint>

template <class T>
class TArray
{
protected:
    T *Data;
    int32_t NumElements;
    int32_t MaxElements;

public:
    TArray(const TArray &) = default;
    TArray(TArray &&) = default;

    inline TArray() : Data(nullptr), NumElements(0), MaxElements(0) {}
    inline TArray(int size) : NumElements(0), MaxElements(size), Data(reinterpret_cast<T *>(calloc(1, sizeof(T) * size))) {}

    TArray &operator=(TArray &&) = default;
    TArray &operator=(const TArray &) = default;

    inline T &operator[](int i) { return (IsValid() && IsValidIndex(i)) ? Data[i] : T(); };
    inline const T &operator[](int i) const { (IsValid() && IsValidIndex(i)) ? Data[i] : T(); }

    inline explicit operator bool() const { return IsValid(); };

    inline bool IsValid() const { return Data != nullptr; }
    inline bool IsValidIndex(int index) const { return index >= 0 && index < NumElements; }

    inline int Slack() const { return MaxElements - NumElements; }

    inline int Num() const { return NumElements; }
    inline int Max() const { return MaxElements; }

    inline T *GetData() const { return Data; }
    inline T *GetDataAt(int index) const { return Data + index; }

    inline bool Add(const T &element)
    {
        if (Slack() <= 0) return false;

        Data[NumElements] = element;
        NumElements++;
        return true;
    }

    inline bool RemoveAt(int index)
    {
        if (!IsValidIndex(index)) return false;

        NumElements--;

        for (int i = index; i < NumElements; i++)
        {
            Data[i] = Data[i + 1];
        }

        return true;
    }

    inline void Clear()
    {
        NumElements = 0;
        if (Data) memset(Data, 0, sizeof(T) * MaxElements);
    }
};

class FString : public TArray<wchar_t>
{
public:
    FString() = default;
    inline FString(const wchar_t *wstr)
    {
        MaxElements = NumElements = (wstr && *wstr) ? int32_t(std::wcslen(wstr)) + 1 : 0;
        if (NumElements) Data = const_cast<wchar_t *>(wstr);
    }

    inline FString operator=(const wchar_t *&&other) { return FString(other); }

    inline std::wstring ToWString() const { return IsValid() ? Data : L""; }
};

template <typename KeyType, typename ValueType>
class TPair
{
private:
    KeyType First;
    ValueType Second;

public:
    TPair() = default;
    inline TPair(KeyType Key, ValueType Value) : First(Key), Second(Value) {}

    inline KeyType &Key() { return First; }
    inline const KeyType &Key() const { return First; }
    inline ValueType &Value() { return Second; }
    inline const ValueType &Value() const { return Second; }
};)";
}