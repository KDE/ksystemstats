/*
 * SPDX-FileCopyrightText: 2024 Henry Hu <henry.hu.sh@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "FreeBSDNvidiaGpu.h"

#include <QRegularExpression>

#include "debug.h"

// FreeBSD's devinfo returns locations like "slot=0 function=0 dbsf=pci0:1:0:0 ...",
// we need to convert it to the format exported by nvidia-smi, like "0000:01:00.0".
QString convertLocationToPciPath(const QString& location) {
    QRegularExpression re("dbsf=pci(\\d+):(\\d+):(\\d+):(\\d+)");
    QRegularExpressionMatch match = re.match(location);
    if (!match.hasMatch()) {
        return "";
    }
    return QString::asprintf("%04x:%02x:%02x.%x", match.captured(1).toInt(), match.captured(2).toInt(), match.captured(3).toInt(), match.captured(4).toInt());
}

FreeBSDNvidiaGpu::FreeBSDNvidiaGpu(const QString &id, const QString &name, const QString &location)
    : NvidiaGpu(id, name, convertLocationToPciPath(location))
{
}

#include "moc_FreeBSDNvidiaGpu.cpp"
