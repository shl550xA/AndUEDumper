#include <cerrno>
#include <chrono>
#include <cstdio>
#include <string>
#include <unordered_map>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>

#include "Utils/Logger.hpp"

#include "Dumper.hpp"

#include "UE/UEMemory.hpp"
#include "UE/UEGameProfile.hpp"

#include "UE/UEGameProfiles/ArenaBreakout.hpp"
#include "UE/UEGameProfiles/DeltaForce.hpp"
#include "UE/UEGameProfiles/Farlight.hpp"
#include "UE/UEGameProfiles/PUBG.hpp"
#include "UE/UEGameProfiles/Valorant.hpp"

std::vector<IGameProfile *> UE_Games = {
    new ArenaBreakoutProfile(),
    new DeltaForceProfile(),
    new FarlightProfile(),
    new PUBGProfile(),
    new ValorantProfile(),
};

#define kUEDUMPER_VERSION "4.2.0"

// increase if needed
#define WAIT_TIME_SEC 20

void dump_thread(bool bDumpLib);

extern "C" void callMe(bool bDumpLib)
{
    LOGI("Starting UEDump3r thread...");
    std::thread(dump_thread, bDumpLib).detach();
}

__attribute__((constructor)) void onload()
{
    LOGI("Using UE Dumper %s", kUEDUMPER_VERSION);
    // callMe(true); // dump lib from memory
    callMe(false);
}

void dump_thread(bool bDumpLib)
{
    LOGI("Dump will start after %d seconds.", WAIT_TIME_SEC);
    sleep(WAIT_TIME_SEC);
    LOGI("==========================");

    std::string sGamePackage = getprogname();
    pid_t gamePID = getpid();

    // dumping at external app data folder to avoid external storage permission
    std::string sOutDirectory = KittyUtils::getExternalStorage();
    sOutDirectory += "/Android/data/";
    sOutDirectory += sGamePackage;
    sOutDirectory += "/files";

    LOGI("Game: %s", sGamePackage.c_str());
    LOGI("Process ID: %d", gamePID);
    LOGI("Output directory: %s", sOutDirectory.c_str());
    LOGI("Dump Library: %s", bDumpLib ? "true" : "false");
    LOGI("==========================");

    std::string sDumpDir = sOutDirectory + "/UEDump3r";
    std::string sDumpGameDir = sDumpDir + "/" + sGamePackage;
    IOUtils::delete_directory(sDumpGameDir);

    if (IOUtils::mkdir_recursive(sDumpGameDir, 0777) == -1)
    {
        int err = errno;
        LOGE("Couldn't create Output Directory [\"%s\"] error=%d | %s.", sDumpDir.c_str(), err, strerror(err));
        return;
    }

    LOGE("Initializing memory...");
    if (!kMgr.initialize(gamePID, EK_MEM_OP_SYSCALL, false) && !kMgr.initialize(gamePID, EK_MEM_OP_IO, false))
    {
        LOGE("Failed to initialize KittyMemoryMgr.");
        return;
    }

    UEDumper uEDumper{};

    uEDumper.setDumpExeInfoNotify([](bool bFinished)
    {
        if (!bFinished)
        {
            LOGI("Dumping Executable Info...");
        }
    });

    uEDumper.setDumpObjectsInfoNotify([](bool bFinished)
    {
        if (!bFinished)
        {
            LOGI("Dumping Objects Info...");
        }
    });

    uEDumper.setDumpOffsetsInfo(false);
    uEDumper.setDumpOffsetsInfoNotify([](bool bFinished)
    {
        if (!bFinished)
        {
            LOGI("Dumping Offsets Info...");
        }
    });

    bool dumpSuccess = false;
    std::unordered_map<std::string, BufferFmt> dumpbuffersMap;
    auto dmpStart = std::chrono::steady_clock::now();

    for (auto &it : UE_Games)
    {
        for (auto &pkg : it->GetAppIDs())
        {
            if (sGamePackage != pkg)
                continue;

            if (bDumpLib)
            {
                auto ue_elf = it->GetUnrealELF();
                if (!ue_elf.isValid())
                {
                    LOGE("Couldn't find a valid UE ELF in target process maps.");
                    return;
                }

                LOGI("Dumping unreal lib from memory...");
                std::string libDumpPath = KittyUtils::String::Fmt("%s/libUE_%p-%p.so", sDumpGameDir.c_str(), ue_elf.base(), ue_elf.end());
                bool res = kMgr.dumpMemELF(ue_elf, libDumpPath);
                LOGI("Dumping lib: %s.", res ? "success" : "failed");
                if (res)
                {
                    LOGI("%s", libDumpPath.c_str());
                }
                LOGI("==========================");
            }

            LOGI("Initializing Dumper...");
            if (uEDumper.Init(it))
            {
                dumpSuccess = uEDumper.Dump(&dumpbuffersMap);
            }

            goto done;
        }
    }

done:

    if (!dumpSuccess && uEDumper.GetLastError().empty())
    {
        LOGE("Game is not supported. check AppID.");
        return;
    }

    if (dumpbuffersMap.empty())
    {
        LOGE("Dump Failed, Error <Buffers empty>");
        LOGE("Dump Status <%s>", uEDumper.GetLastError().c_str());
        return;
    }

    LOGI("Saving Files...");

    for (const auto &it : dumpbuffersMap)
    {
        if (!it.first.empty())
        {
            std::string path = KittyUtils::String::Fmt("%s/%s", sDumpGameDir.c_str(), it.first.c_str());
            it.second.writeBufferToFile(path);
        }
    }

    auto dmpEnd = std::chrono::steady_clock::now();
    std::chrono::duration<float, std::milli> dmpDurationMS = (dmpEnd - dmpStart);

    if (!uEDumper.GetLastError().empty())
    {
        LOGI("Dump Status: %s", uEDumper.GetLastError().c_str());
    }
    LOGI("Dump Duration: %.2fms", dmpDurationMS.count());
    LOGI("Dump Location: %s", sDumpGameDir.c_str());
}