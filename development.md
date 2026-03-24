# KSystemStats Development Guide

## GPU Monitoring Plugins

When developing or running a local build of KSystemStats (e.g., via `kde-builder`), you might notice that GPU sensors report `0` or fail to retrieve data. This is typically caused by missing dependencies or a lack of the elevated permissions required to read hardware performance counters.

### NVIDIA GPUs
The NVIDIA backend relies on parsing the output of the `nvidia-smi` command-line utility. 
If your sensors are reporting 0 and you see `Could not retrieve information for NVidia GPU` in the journal logs, ensure that `nvidia-smi` is installed on your system and available in your `$PATH`. It is usually provided by packages like `nvidia-utils` or `xorg-x11-drv-nvidia-cuda` depending on your Linux distribution.

### Intel GPUs
The Intel backend uses a dedicated helper executable (`ksystemstats_intel_helper`) to read performance events from the kernel via the `i915` DRM interface. 

Accessing these performance counters requires the `CAP_PERFMON` (or `CAP_SYS_ADMIN` on older kernels) Linux capability. System-wide installations handle this automatically as root. However, local builds performed by a regular user (like `kde-builder`) cannot set capabilities. As a result, the helper will fail with an error similar to `"Failed opening any event\n"` in the journal, and the sensors will report 0.

To fix this for your local development build, you must manually grant the required capability to the compiled helper using `sudo`:

```bash
# Locate your built ksystemstats_intel_helper and apply the capability
sudo setcap cap_perfmon=+ep /path/to/your/build/prefix/libexec/ksystemstats_intel_helper
```
*(Example: `sudo setcap cap_perfmon=+ep ~/kde/usr/libexec/ksystemstats_intel_helper`)*

After applying the capability, restart your local daemon for the changes to take effect:
```bash
systemctl --user restart plasma-ksystemstats.service
```
<!--
SPDX-FileCopyrightText: 2026 Bernhard Friedreich <friesoft@gmail.com>
SPDX-License-Identifier: BSD-2-Clause
-->
