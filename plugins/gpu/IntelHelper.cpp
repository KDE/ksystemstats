/*
 * SPDX-FileCopyrightText: 2021 David Redondo <kde@david-redondo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <array>
#include <chrono>
#include <cinttypes>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <map>
#include <iostream>

#include <drm/i915_drm.h>
#include <linux/perf_event.h>
#include <sys/capability.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <unistd.h>

constexpr auto eventSourceDir = "/sys/bus/event_source/devices/i915";

int i915Type()
{
    const std::string path = eventSourceDir + std::string("/type");
    std::ifstream typeFile(path);
    if (!typeFile.is_open()) {
        std::cerr << "Could not open " << path;
        std::exit(1);
    }
    int type = -1;
    if(!(typeFile >> type)) {
        std::cerr << "Error reading type";
        std::exit(1);
    }
    return type;
}

int perf_open(int type, int config, int group_fd)
{
    perf_event_attr pe;
    std::memset(&pe, 0, sizeof(pe));

    pe.type = type;
    pe.size = sizeof(pe);
    pe.config = config;
    pe.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_ID | PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_ENABLED;

    return syscall(SYS_perf_event_open, &pe, -1, 0, group_fd, PERF_FLAG_FD_CLOEXEC);
}

template <int n>
struct read_format {
    std::uint64_t count;
    std::uint64_t time_enabled;
    struct value {
        std::uint64_t value;
        std::uint64_t id;
    };
    std::array<value, n> values;
};

int main()
{
    const int type = i915Type();
    int group_fd = -1;
    // TODO Should we also add sema and wait usages?
    constexpr std::array events = {I915_PMU_INTERRUPTS,
                                   I915_PMU_ACTUAL_FREQUENCY,
                                   I915_PMU_ENGINE_BUSY(I915_ENGINE_CLASS_RENDER, 0),
                                   I915_PMU_ENGINE_BUSY(I915_ENGINE_CLASS_COPY, 0),
                                   I915_PMU_ENGINE_BUSY(I915_ENGINE_CLASS_VIDEO, 0),
                                   I915_PMU_ENGINE_BUSY(I915_ENGINE_CLASS_VIDEO_ENHANCE, 0)};
    std::map<std::uint64_t, int> idToEvent;
    for (const auto event : events) {
        const int fd = perf_open(type, event, group_fd);
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

    read_format<events.size()> data;
    while (true) {
        if (read(group_fd, &data, sizeof(data)) < 0) {
            std::cerr << "Error reading events" << std::endl;
            return errno;
        }
        std::cout << data.time_enabled;
        for (auto value = data.values.cbegin(); value != data.values.cbegin() + data.count; ++value) {
            if (!idToEvent.count(value->id)) {
                std::cerr << "Unknown event id" << value->id << "\n";
                continue;
            }
            switch (idToEvent[value->id]) {
            case I915_PMU_INTERRUPTS:
                std::cout << "|Interrupts";
                break;
            case I915_PMU_ACTUAL_FREQUENCY:
                std::cout << "|Frequency";
                break;
            case I915_PMU_ENGINE_BUSY(I915_ENGINE_CLASS_RENDER, 0):
                std::cout << "|Render";
                break;
            case I915_PMU_ENGINE_BUSY(I915_ENGINE_CLASS_COPY, 0):
                std::cout << "|Copy";
                break;
            case I915_PMU_ENGINE_BUSY(I915_ENGINE_CLASS_VIDEO, 0):
                std::cout << "|Video";
                break;
            case I915_PMU_ENGINE_BUSY(I915_ENGINE_CLASS_VIDEO_ENHANCE, 0):
                std::cout << "|Enhance";
                break;
            }
            std::cout << "|" << value->value;
        }
        std::cout << std::endl;
        sleep(1);
    }
}
