/*
 * SPDX-FileCopyrightText: 2021 David Redondo <kde@david-redondo.de>
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

#include <libdrm/i915_drm.h>
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>

constexpr auto igpuPciId = "0000:00:02.0";

struct engine_info {
    drm_i915_gem_engine_class type;
    std::uint64_t config;
};

int perf_open(int type, int config, int group_fd)
{
    perf_event_attr pe;
    std::memset(&pe, 0, sizeof(pe));

    pe.type = type;
    pe.size = sizeof(pe);
    pe.config = config;
    pe.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_ID | PERF_FORMAT_TOTAL_TIME_ENABLED;

    return syscall(SYS_perf_event_open, &pe, -1, 0, group_fd, PERF_FLAG_FD_CLOEXEC);
}

std::vector<engine_info> discoverEngines(const std::filesystem::path &deviceDir)
{
    std::vector<engine_info> engines;
    for (const auto &entry : std::filesystem::directory_iterator(deviceDir / "events", std::filesystem::directory_options::skip_permission_denied)) {
        if (!entry.is_regular_file() || !entry.path().string().ends_with("busy")) {
            continue;
        }
        std::ifstream configFile(entry.path());
        if (!configFile.is_open()) {
            std::cerr << "Could not open " << entry.path() << '\n';
            continue;
        }
        std::string contents;
        if (!std::getline(configFile, contents)) {
            std::cerr << "Could not read " << entry.path() << '\n';
            continue;
        }
        if (!contents.starts_with("config=")) {
            std::cerr << entry.path() << ": Expected 'config=' got " << contents << '\n';
            continue;
        }
        const std::uint64_t config = std::strtol(&contents.at(strlen("config=")), nullptr, 0);
        if (errno != 0) {
            std::cerr << entry.path() << ": Failed parsing" << contents << '\n';
            continue;
        }
        // Reverse of I915_PMU_ENGINE_BUSY macro
        if ((config & I915_PMU_SAMPLE_MASK) != I915_SAMPLE_BUSY) {
            std::cerr << entry.path() << ": Config is not a busy sample" << config << '\n';
            continue;
        }
        const int type = config >> I915_PMU_CLASS_SHIFT;
        if (type != I915_ENGINE_CLASS_RENDER && type != I915_ENGINE_CLASS_COPY && type != I915_ENGINE_CLASS_VIDEO
            && type != I915_ENGINE_CLASS_VIDEO_ENHANCE) {
            std::cerr << entry.path() << ": " << type << " is not an engine class \n";
            continue;
        }
        engines.push_back({static_cast<drm_i915_gem_engine_class>(type), config});
    }
    return engines;
}

int readType(const std::filesystem::path &deviceDir)
{
    const auto filePath = deviceDir / "type";
    std::ifstream typeFile(filePath);
    if (!typeFile.is_open()) {
        std::cerr << "Could not open " << filePath << '\n';
        std::exit(1);
    }
    int type = -1;
    if (!(typeFile >> type)) {
        std::cerr << "Error reading type from " << filePath << '\n';
        std::exit(1);
    }
    return type;
}

std::filesystem::path deviceDirectory(std::string_view pciId)
{
    // fileName is i915_xxxx_yy_zz.z where pci address is xxxx:yy:zz.z
    // No pci address in filename is integrated GPU with address of "0000:00:02.0"
    // see intel-gpu-top https://gitlab.freedesktop.org/drm/igt-gpu-tools/-/blob/17707095f1e5d3c30f463b43022f01c0160579b6/tools/intel_gpu_top.c?page=3#L2291
    constexpr auto dirPrefix = "/sys/bus/event_source/devices/i915";
    if (pciId == igpuPciId) {
        return dirPrefix;
    }
    std::string pciSuffix(pciId);
    std::replace(pciSuffix.begin(), pciSuffix.end(), ':', '_');
    return dirPrefix + pciSuffix;
}

int main(int argc, char **argv)
{
    const char *requestedDevice = argc > 1 ? argv[1] : igpuPciId;
    const auto deviceDir = deviceDirectory(requestedDevice);
    if (!(std::filesystem::exists(deviceDir) && std::filesystem::is_directory(deviceDir))) {
        std::cerr << "Device directory " << deviceDir << " does not exist\n";
        exit(1);
    }

    const int perfType = readType(deviceDir);
    const auto engines = discoverEngines(deviceDir);
    std::array<size_t, 4> enginesPerClass{};
    for (const auto &engine : engines) {
        ++enginesPerClass[engine.type];
    }

    std::vector<std::uint64_t> events;
    events.reserve(engines.size() + 2);
    std::ranges::transform(engines, std::back_inserter(events), std::mem_fn(&engine_info::config));
    events.insert(events.end(), {I915_PMU_ACTUAL_FREQUENCY, I915_PMU_INTERRUPTS});

    int group_fd = -1;
    std::map<std::uint64_t, uint64_t> idToEvent;

    for (const auto event : events) {
        const int fd = perf_open(perfType, event, group_fd);
        if (group_fd == -1) {
            group_fd = fd;
        }
        if (fd != -1) {
            std::uint64_t id;
            ioctl(fd, PERF_EVENT_IOC_ID, &id);
            idToEvent.emplace(id, event);
        }
    }

    if (group_fd == -1) {
        std::cerr << "Failed opening any event" << std::endl;
        return -1;
    }

    struct read_format {
        std::uint64_t count;
        std::uint64_t time_enabled;
        struct value {
            std::uint64_t value;
            std::uint64_t id;
        } values[];
    };
    const size_t neededSize = sizeof(read_format) + sizeof(read_format::value) * idToEvent.size();
    read_format *data = static_cast<read_format *>(operator new(neededSize));
    std::array<std::uint64_t, 4> engineCounters;
    while (true) {
        if (read(group_fd, data, neededSize) < 0) {
            std::cerr << "Error reading events" << std::endl;
            return errno;
        }
        engineCounters = {};
        std::cout << data->time_enabled;
        for (std::uint64_t i = 0; i < data->count; ++i) {
            const auto event = idToEvent.find(data->values[i].id);
            if (event == idToEvent.end()) {
                std::cerr << "Unknown event id" << data->values[i].id << "\n";
                continue;
            }
            switch (event->second) {
            case I915_PMU_INTERRUPTS:
                std::cout << "|Interrupts|" << data->values[i].value;
                break;
            case I915_PMU_ACTUAL_FREQUENCY:
                std::cout << "|Frequency|" << data->values[i].value;
                break;
            default: {
                engineCounters[event->second >> I915_PMU_CLASS_SHIFT] += data->values[i].value;
            }
            }
        }

        std::cout << "|Render|" << engineCounters[I915_ENGINE_CLASS_RENDER] / enginesPerClass[I915_ENGINE_CLASS_RENDER];
        std::cout << "|Copy|" << engineCounters[I915_ENGINE_CLASS_COPY] / enginesPerClass[I915_ENGINE_CLASS_COPY];
        std::cout << "|Video|" << engineCounters[I915_ENGINE_CLASS_VIDEO] / enginesPerClass[I915_ENGINE_CLASS_VIDEO];
        std::cout << "|Enhance|" << engineCounters[I915_ENGINE_CLASS_VIDEO_ENHANCE] / enginesPerClass[I915_ENGINE_CLASS_VIDEO_ENHANCE];

        std::cout << std::endl;
        sleep(1);
    }
}
