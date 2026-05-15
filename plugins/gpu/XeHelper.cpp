/*
 * SPDX-FileCopyrightText: 2021 David Redondo <kde@david-redondo.de>
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
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>

// Intel Xe PMU (Performance Monitoring Unit) constants
// Reference: drivers/gpu/drm/xe/xe_pmu.c and drivers/gpu/drm/xe/xe_pmu.h in Linux kernel
// GT (Graphics Technology) refers to the GPU tile - modern Intel GPUs can have multiple GTs
// Engine classes: 0=Render, 1=Copy, 2=Video, 3=VideoEnhance

// Event types
constexpr std::uint64_t XE_PMU_EVENT_ENGINE_ACTIVE_TICKS = 0x02;
constexpr std::uint64_t XE_PMU_EVENT_ENGINE_TOTAL_TICKS = 0x03;
constexpr std::uint64_t XE_PMU_EVENT_GT_ACTUAL_FREQUENCY = 0x04;

// Config field layout for perf_event_attr.config
constexpr int XE_PMU_EVENT_SHIFT = 0;              // Bits 0-11: Event type
constexpr int XE_PMU_ENGINE_INSTANCE_SHIFT = 12;   // Bits 12-19: Engine instance
constexpr int XE_PMU_ENGINE_CLASS_SHIFT = 20;      // Bits 20-59: Engine class
constexpr int XE_PMU_GT_SHIFT = 60;                // Bits 60-63: GT (tile) ID

struct engine_info {
    int engine_class;
    int engine_instance;
    int gt;
    std::uint64_t active_config;
    std::uint64_t total_config;
};

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

std::vector<engine_info> discoverEngines(int perfType)
{
    std::vector<engine_info> engines;

    // Try discovering engines by attempting to open perf events
    // Engine classes: 0=Render, 1=Copy, 2=Video, 3=VideoEnhance (4 known classes)
    // GT and instance counts may vary by GPU, so probe until we fail
    constexpr int maxGts = 8;
    constexpr int maxInstances = 16;
    for (int gt = 0; gt < maxGts; ++gt) {
        bool foundAnyInGt = false;
        for (int engine_class = 0; engine_class <= 3; ++engine_class) {
            for (int instance = 0; instance < maxInstances; ++instance) {
                engine_info info;
                info.engine_class = engine_class;
                info.engine_instance = instance;
                info.gt = gt;
                info.active_config = makeConfig(XE_PMU_EVENT_ENGINE_ACTIVE_TICKS, engine_class, instance, gt);
                info.total_config = makeConfig(XE_PMU_EVENT_ENGINE_TOTAL_TICKS, engine_class, instance, gt);

                int testFd = perf_open(perfType, info.active_config, -1);
                if (testFd >= 0) {
                    close(testFd);
                    engines.push_back(info);
                    foundAnyInGt = true;
                } else {
                    break;
                }
            }
        }
        if (!foundAnyInGt) {
            break;
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
    // Xe uses /sys/bus/event_source/devices/xe_<pci_address> format
    // PCI address uses underscores instead of colons, e.g. xe_0000_03_00.0
    constexpr auto basePath = "/sys/bus/event_source/devices/";

    std::string pciSuffix(pciSlot);
    std::replace(pciSuffix.begin(), pciSuffix.end(), ':', '_');
    return std::filesystem::path(basePath) / ("xe_" + pciSuffix);
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <pci_address>\n";
        return 1;
    }

    // Validate PCI address format (DDDD:BB:DD.F)
    static const std::regex pciPattern(R"([0-9a-fA-F]{4}:[0-9a-fA-F]{2}:[0-9a-fA-F]{2}\.[0-9a-fA-F])");
    if (!std::regex_match(argv[1], pciPattern)) {
        std::cerr << "Invalid PCI address format: " << argv[1] << "\n";
        return 1;
    }

    const auto deviceDir = deviceDirectory(argv[1]);

    if (!(std::filesystem::exists(deviceDir) && std::filesystem::is_directory(deviceDir))) {
        std::cerr << "Device directory " << deviceDir << " does not exist\n";
        return 1;
    }

    const int perfType = readType(deviceDir);
    if (perfType < 0) {
        return 1;
    }

    const auto engines = discoverEngines(perfType);

    std::vector<std::uint64_t> events;

    events.push_back(makeConfig(XE_PMU_EVENT_GT_ACTUAL_FREQUENCY));

    for (const auto &engine : engines) {
        events.push_back(engine.active_config);
        events.push_back(engine.total_config);
    }

    int group_fd = -1;
    std::map<std::uint64_t, std::uint64_t> idToConfig;
    std::set<std::uint64_t> openedConfigs;

    std::vector<int> openedFds;
    for (const auto config : events) {
        const int fd = perf_open(perfType, config, group_fd);
        if (fd == -1) {
            continue;
        }
        if (group_fd == -1) {
            group_fd = fd;
        }
        openedFds.push_back(fd);
        std::uint64_t id = 0;
        if (ioctl(fd, PERF_EVENT_IOC_ID, &id) < 0) {
            continue;
        }
        idToConfig.emplace(id, config);
        openedConfigs.insert(config);
    }

    if (group_fd == -1) {
        std::cerr << "Failed opening any event" << std::endl;
        return -1;
    }

    std::array<int, 4> enginesPerClass{};
    std::array<std::vector<std::pair<std::uint64_t, std::uint64_t>>, 4> engineConfigs;

    for (const auto &engine : engines) {
        if (openedConfigs.count(engine.active_config) && openedConfigs.count(engine.total_config)) {
            enginesPerClass[engine.engine_class]++;
            engineConfigs[engine.engine_class].push_back({engine.active_config, engine.total_config});
        }
    }

    struct read_format {
        std::uint64_t count;
        std::uint64_t time_enabled;  // Used to calculate time deltas for frequency measurements
        struct value {
            std::uint64_t value;
            std::uint64_t id;
        } values[];
    };

    const size_t neededSize = sizeof(read_format) + sizeof(read_format::value) * idToConfig.size();
    std::vector<char> buffer(neededSize);
    read_format *data = reinterpret_cast<read_format *>(buffer.data());

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

        auto freqConfig = makeConfig(XE_PMU_EVENT_GT_ACTUAL_FREQUENCY);
        auto freqIt = currentValues.find(freqConfig);
        if (freqIt != currentValues.end()) {
            auto lastIt = lastValues.find(freqConfig);
            if (lastIt != lastValues.end() && lastTimestamp > 0
                && data->time_enabled >= lastTimestamp && freqIt->second >= lastIt->second) {
                std::uint64_t timeDiff = data->time_enabled - lastTimestamp;
                std::uint64_t freqDiff = freqIt->second - lastIt->second;
                if (timeDiff > 0) {
                    std::uint64_t avgFreq = (freqDiff * 1000000000ULL) / timeDiff;
                    std::cout << "|Frequency|" << avgFreq;
                }
            }
        }

        // Only expose Render (class 0) and Video (class 2) to match i915 parity
        constexpr int exposedClasses[] = {0, 2};
        const char* classNames[] = {"Render", "Copy", "Video", "Enhance"};

        for (int engineClass : exposedClasses) {
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
                    lastActiveIt != lastValues.end() && lastTotalIt != lastValues.end() &&
                    activeIt->second >= lastActiveIt->second && totalIt->second >= lastTotalIt->second) {
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
