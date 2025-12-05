/*
 * SPDX-FileCopyrightText: 2025 Hunter Hardy <thehunterhardy@icloud.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include <fcntl.h>
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <drm/xe_drm.h>

constexpr std::uint64_t XE_PMU_EVENT_ENGINE_ACTIVE_TICKS = 0x02;
constexpr std::uint64_t XE_PMU_EVENT_ENGINE_TOTAL_TICKS = 0x03;
constexpr std::uint64_t XE_PMU_EVENT_GT_ACTUAL_FREQUENCY = 0x04;

constexpr int XE_PMU_EVENT_SHIFT = 0;
constexpr int XE_PMU_ENGINE_INSTANCE_SHIFT = 12;
constexpr int XE_PMU_ENGINE_CLASS_SHIFT = 20;
constexpr int XE_PMU_GT_SHIFT = 60;

struct engine_info {
    int engine_class;
    int engine_instance;
    int gt;
    std::uint64_t active_config;
    std::uint64_t total_config;
};

struct vram_info {
    std::uint64_t total;
    std::uint64_t used;
};

int openDrmDevice(std::string_view pciSlot)
{
    for (int i = 0; i < 16; ++i) {
        std::string cardPath = "/dev/dri/card" + std::to_string(i);
        int fd = open(cardPath.c_str(), O_RDONLY);
        if (fd < 0) {
            continue;
        }

        std::string sysPath = "/sys/class/drm/card" + std::to_string(i) + "/device";
        char linkTarget[256];
        ssize_t len = readlink(sysPath.c_str(), linkTarget, sizeof(linkTarget) - 1);
        if (len > 0) {
            linkTarget[len] = '\0';
            std::string target(linkTarget);
            if (target.find(pciSlot) != std::string::npos) {
                return fd;
            }
        }
        close(fd);
    }
    return -1;
}

vram_info queryVram(int drmFd)
{
    vram_info info{0, 0};
    if (drmFd < 0) {
        return info;
    }

    drm_xe_device_query query{};
    query.query = DRM_XE_DEVICE_QUERY_MEM_REGIONS;
    query.size = 0;
    query.data = 0;

    if (ioctl(drmFd, DRM_IOCTL_XE_DEVICE_QUERY, &query) != 0 || query.size == 0) {
        return info;
    }

    std::vector<char> buffer(query.size);
    query.data = reinterpret_cast<std::uint64_t>(buffer.data());

    if (ioctl(drmFd, DRM_IOCTL_XE_DEVICE_QUERY, &query) != 0) {
        return info;
    }

    auto *regions = reinterpret_cast<drm_xe_query_mem_regions *>(buffer.data());
    for (std::uint32_t i = 0; i < regions->num_mem_regions; ++i) {
        if (regions->mem_regions[i].mem_class == DRM_XE_MEM_REGION_CLASS_VRAM) {
            info.total += regions->mem_regions[i].total_size;
            info.used += regions->mem_regions[i].used;
        }
    }

    return info;
}

int perf_open(int type, std::uint64_t config, int group_fd)
{
    perf_event_attr pe;
    std::memset(&pe, 0, sizeof(pe));

    pe.type = type;
    pe.size = sizeof(pe);
    pe.config = config;
    pe.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_ID | PERF_FORMAT_TOTAL_TIME_ENABLED;

    return syscall(SYS_perf_event_open, &pe, -1, 0, group_fd, PERF_FLAG_FD_CLOEXEC);
}

std::uint64_t makeConfig(int event, int engine_class = 0, int engine_instance = 0, int gt = 0)
{
    return (static_cast<std::uint64_t>(event) << XE_PMU_EVENT_SHIFT)
         | (static_cast<std::uint64_t>(engine_instance) << XE_PMU_ENGINE_INSTANCE_SHIFT)
         | (static_cast<std::uint64_t>(engine_class) << XE_PMU_ENGINE_CLASS_SHIFT)
         | (static_cast<std::uint64_t>(gt) << XE_PMU_GT_SHIFT);
}

std::vector<engine_info> discoverEngines()
{
    std::vector<engine_info> engines;

    for (int gt = 0; gt < 2; ++gt) {
        for (int engine_class = 0; engine_class <= 3; ++engine_class) {
            for (int instance = 0; instance < 4; ++instance) {
                engine_info info;
                info.engine_class = engine_class;
                info.engine_instance = instance;
                info.gt = gt;
                info.active_config = makeConfig(XE_PMU_EVENT_ENGINE_ACTIVE_TICKS, engine_class, instance, gt);
                info.total_config = makeConfig(XE_PMU_EVENT_ENGINE_TOTAL_TICKS, engine_class, instance, gt);
                engines.push_back(info);
            }
        }
    }

    return engines;
}

int readType(const std::filesystem::path &deviceDir)
{
    const auto filePath = deviceDir / "type";
    std::ifstream typeFile(filePath);
    if (!typeFile.is_open()) {
        std::cerr << "Could not open " << filePath << '\n';
        return -1;
    }
    int type = -1;
    if (!(typeFile >> type)) {
        std::cerr << "Error reading type from " << filePath << '\n';
        return -1;
    }
    return type;
}

std::filesystem::path deviceDirectory(std::string_view pciSlot)
{
    // Xe uses /sys/bus/event_source/devices/xe_0000_03_00.0 format
    constexpr auto basePath = "/sys/bus/event_source/devices/";

    std::string pciSuffix(pciSlot);
    std::replace(pciSuffix.begin(), pciSuffix.end(), ':', '_');
    auto specificPath = std::filesystem::path(basePath) / ("xe_" + pciSuffix);
    if (std::filesystem::exists(specificPath)) {
        return specificPath;
    }

    if (pciSlot.find(':') != std::string_view::npos && pciSlot.length() < 12) {
        std::string fullSlot = "0000_" + std::string(pciSlot);
        std::replace(fullSlot.begin(), fullSlot.end(), ':', '_');
        auto fullPath = std::filesystem::path(basePath) / ("xe_" + fullSlot);
        if (std::filesystem::exists(fullPath)) {
            return fullPath;
        }
    }

    auto fallbackPath = std::filesystem::path(basePath) / "xe";
    if (std::filesystem::exists(fallbackPath)) {
        return fallbackPath;
    }

    for (const auto &entry : std::filesystem::directory_iterator(basePath)) {
        if (entry.is_directory() && entry.path().filename().string().starts_with("xe_")) {
            return entry.path();
        }
    }

    return std::filesystem::path(basePath) / "xe";
}

int main(int argc, char **argv)
{
    const char *requestedDevice = argc > 1 ? argv[1] : "0000:03:00.0";
    const auto deviceDir = deviceDirectory(requestedDevice);

    if (!(std::filesystem::exists(deviceDir) && std::filesystem::is_directory(deviceDir))) {
        std::cerr << "Device directory " << deviceDir << " does not exist\n";
        return 1;
    }

    const int perfType = readType(deviceDir);
    if (perfType < 0) {
        return 1;
    }

    const auto engines = discoverEngines();
    const int drmFd = openDrmDevice(requestedDevice);

    std::vector<std::uint64_t> events;

    events.push_back(makeConfig(XE_PMU_EVENT_GT_ACTUAL_FREQUENCY));

    for (const auto &engine : engines) {
        events.push_back(engine.active_config);
        events.push_back(engine.total_config);
    }

    int group_fd = -1;
    std::map<std::uint64_t, std::uint64_t> idToConfig;
    std::map<std::uint64_t, int> configToFd;

    for (const auto config : events) {
        const int fd = perf_open(perfType, config, group_fd);
        if (fd == -1) {
            continue;
        }
        if (group_fd == -1) {
            group_fd = fd;
        }
        std::uint64_t id;
        ioctl(fd, PERF_EVENT_IOC_ID, &id);
        idToConfig.emplace(id, config);
        configToFd.emplace(config, fd);
    }

    if (group_fd == -1) {
        std::cerr << "Failed opening any event" << std::endl;
        return -1;
    }

    std::array<int, 4> enginesPerClass{};
    std::array<std::vector<std::pair<std::uint64_t, std::uint64_t>>, 4> engineConfigs;

    for (const auto &engine : engines) {
        auto activeIt = configToFd.find(engine.active_config);
        auto totalIt = configToFd.find(engine.total_config);
        if (activeIt != configToFd.end() && totalIt != configToFd.end()) {
            enginesPerClass[engine.engine_class]++;
            engineConfigs[engine.engine_class].push_back({engine.active_config, engine.total_config});
        }
    }

    struct read_format {
        std::uint64_t count;
        std::uint64_t time_enabled;
        struct value {
            std::uint64_t value;
            std::uint64_t id;
        } values[];
    };

    const size_t neededSize = sizeof(read_format) + sizeof(read_format::value) * idToConfig.size();
    read_format *data = static_cast<read_format *>(operator new(neededSize));

    std::map<std::uint64_t, std::uint64_t> lastValues;
    std::uint64_t lastTimestamp = 0;

    while (true) {
        if (read(group_fd, data, neededSize) < 0) {
            std::cerr << "Error reading events" << std::endl;
            return errno;
        }

        std::map<std::uint64_t, std::uint64_t> currentValues;
        for (std::uint64_t i = 0; i < data->count; ++i) {
            auto it = idToConfig.find(data->values[i].id);
            if (it != idToConfig.end()) {
                currentValues[it->second] = data->values[i].value;
            }
        }

        std::cout << data->time_enabled;

        auto vram = queryVram(drmFd);
        if (vram.total > 0) {
            std::cout << "|VramTotal|" << vram.total << "|VramUsed|" << vram.used;
        }

        auto freqConfig = makeConfig(XE_PMU_EVENT_GT_ACTUAL_FREQUENCY);
        auto freqIt = currentValues.find(freqConfig);
        if (freqIt != currentValues.end()) {
            auto lastIt = lastValues.find(freqConfig);
            if (lastIt != lastValues.end() && lastTimestamp > 0) {
                std::uint64_t timeDiff = data->time_enabled - lastTimestamp;
                std::uint64_t freqDiff = freqIt->second - lastIt->second;
                if (timeDiff > 0) {
                    std::uint64_t avgFreq = (freqDiff * 1000000000ULL) / timeDiff;
                    std::cout << "|Frequency|" << avgFreq;
                }
            }
        }

        const char* classNames[] = {"Render", "Copy", "Video", "Enhance"};

        for (int engineClass = 0; engineClass < 4; ++engineClass) {
            if (enginesPerClass[engineClass] == 0) {
                continue;
            }

            std::uint64_t totalActive = 0;
            std::uint64_t totalTicks = 0;
            int validEngines = 0;

            for (const auto &[activeConfig, totalConfig] : engineConfigs[engineClass]) {
                auto activeIt = currentValues.find(activeConfig);
                auto totalIt = currentValues.find(totalConfig);
                auto lastActiveIt = lastValues.find(activeConfig);
                auto lastTotalIt = lastValues.find(totalConfig);

                if (activeIt != currentValues.end() && totalIt != currentValues.end() &&
                    lastActiveIt != lastValues.end() && lastTotalIt != lastValues.end()) {
                    std::uint64_t activeDiff = activeIt->second - lastActiveIt->second;
                    std::uint64_t totalDiff = totalIt->second - lastTotalIt->second;
                    if (totalDiff > 0) {
                        totalActive += activeDiff;
                        totalTicks += totalDiff;
                        validEngines++;
                    }
                }
            }

            if (validEngines > 0 && totalTicks > 0) {
                double usage = (static_cast<double>(totalActive) / totalTicks) * 100.0;
                std::cout << "|" << classNames[engineClass] << "|" << static_cast<std::uint64_t>(usage);
            }
        }

        std::cout << std::endl;

        lastValues = currentValues;
        lastTimestamp = data->time_enabled;

        sleep(1);
    }
}
