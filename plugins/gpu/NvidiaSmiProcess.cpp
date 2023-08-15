/*
 * SPDX-FileCopyrightText: 2019 David Edmundson <davidedmundson@kde.org>
 * SPDX-FileCopyrightText: 2020 Arjen Hiemstra <ahiemstra@heimr.nl>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "NvidiaSmiProcess.h"

#include <QStandardPaths>

NvidiaSmiProcess::NvidiaSmiProcess()
{
    m_smiPath = QStandardPaths::findExecutable(QStringLiteral("nvidia-smi"));
}

bool NvidiaSmiProcess::isSupported() const
{
    return !m_smiPath.isEmpty();
}

std::vector<NvidiaSmiProcess::GpuQueryResult> NvidiaSmiProcess::query()
{
    if (!isSupported()) {
        return m_queryResult;
    }

    if (!m_queryResult.empty()) {
        return m_queryResult;
    }

    // Read and parse the result of "nvidia-smi query"
    // This seems to be the only way to get certain values like total memory or
    // maximum temperature. Unfortunately the output isn't very easily parseable
    // so we have to do some trickery to parse things.

    QProcess queryProcess;
    queryProcess.setProgram(m_smiPath);
    queryProcess.setArguments({QStringLiteral("--query")});
    queryProcess.start();

    int gpuCounter = 0;
    auto data = m_queryResult.end();

    bool readMemory = false;
    bool readMaxClocks = false;
    bool readMaxPwr = false;

    while (queryProcess.waitForReadyRead()) {
        if (!queryProcess.canReadLine()) {
            continue;
        }

        auto line = queryProcess.readLine();
        if (line.startsWith("GPU ")) {
            // Start of GPU properties block. Ensure we have a new data object
            // to write to.
            data = m_queryResult.emplace(m_queryResult.end());
            // nvidia-smi has to much zeros compared to linux, remove line break
            data->pciPath = line.mid(strlen("GPU 0000")).chopped(1).toLower();
            gpuCounter++;
        }

        if ((readMemory || readMaxClocks) && !line.startsWith("        ")) {
            // Memory/clock information does not have a unique prefix but should
            // be indented more than their "headers". So if the indentation is
            // less, we are no longer in an info block and should treat it as
            // such.
            readMemory = false;
            readMaxClocks = false;
        }

        if (line.startsWith("    Product Name")) {
            data->name = line.mid(line.indexOf(':') + 1).trimmed();
        }

        if (line.startsWith("    FB Memory Usage") || line.startsWith("    BAR1 Memory Usage")) {
            readMemory = true;
        }

        if (line.startsWith("    Max Clocks")) {
            readMaxClocks = true;
        }

        if (line.startsWith("    Power Readings")) {
            readMaxPwr = true;
        }

        if (line.startsWith("        Total") && readMemory) {
            data->totalMemory += std::atoi(line.mid(line.indexOf(':') + 1));
        }

        if (line.startsWith("        GPU Shutdown Temp")) {
            data->maxTemperature = std::atoi(line.mid(line.indexOf(':') + 1));
        }

        if (line.startsWith("        Graphics") && readMaxClocks) {
            data->maxCoreFrequency = std::atoi(line.mid(line.indexOf(':') + 1));
        }

        if (line.startsWith("        Memory") && readMaxClocks) {
            data->maxMemoryFrequency = std::atoi(line.mid(line.indexOf(':') + 1));
        }

        if (line.startsWith("        Power Limit") && readMaxPwr) {
            data->maxPower = std::atoi(line.mid(line.indexOf(':') + 1));
        }
    }

    return m_queryResult;
}

void NvidiaSmiProcess::ref()
{
    if (!isSupported()) {
        return;
    }

    m_references++;

    if (m_process) {
        return;
    }

    m_process = std::make_unique<QProcess>();
    m_process->setProgram(m_smiPath);
    m_process->setArguments({
        QStringLiteral("dmon"), // Monitor
        QStringLiteral("-d"),
        QStringLiteral("2"), // 2 seconds delay, to match daemon update rate
        QStringLiteral("-s"),
        QStringLiteral("pucm") // Include all relevant statistics
    });
    connect(m_process.get(), &QProcess::readyReadStandardOutput, this, &NvidiaSmiProcess::readStatisticsData);
    m_process->start();
}

void NvidiaSmiProcess::unref()
{
    if (!isSupported()) {
        return;
    }

    m_references--;

    if (!m_process || m_references > 0) {
        return;
    }

    m_process->terminate();
    m_process->waitForFinished();
    m_process.reset();
}

void NvidiaSmiProcess::readStatisticsData()
{
    while (m_process->canReadLine()) {
        QString line = m_process->readLine();
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        QVector<QStringRef> parts = QStringRef(&line).trimmed().split(QLatin1Char(' '), Qt::SkipEmptyParts);
#else
        QVector<QStringView> parts = QStringView(line).trimmed().split(QLatin1Char(' '), Qt::SkipEmptyParts);
#endif
        // Because in Qt5 indexOf of QVector<T> only takes T's, write our own indexOf taking arbitrary types
        auto indexOf = [](const auto &stack, const auto& needle) {
            auto it = std::find(stack.cbegin(), stack.cend(), needle);
            return it != stack.cend() ? std::distance(stack.cbegin(), it) : -1;
        };

        // discover index of fields in the header format is something like
        //# gpu   pwr gtemp mtemp    sm   mem   enc   dec  mclk  pclk    fb  bar1
        // # Idx     W     C     C     %     %     %     %   MHz   MHz    MB    MB
        // 0     25     29      -     1      1      0      0   4006   1506    891     22
        if (line.startsWith(QLatin1Char('#'))) {
            if (m_dmonIndices.gpu == -1) {
                // Remove First part because of leading '# ';
                parts.removeFirst();
                m_dmonIndices.gpu = indexOf(parts, QLatin1String("gpu"));
                m_dmonIndices.power = indexOf(parts, QLatin1String("pwr"));
                m_dmonIndices.gtemp = indexOf(parts, QLatin1String("gtemp"));
                m_dmonIndices.sm = indexOf(parts, QLatin1String("sm"));
                m_dmonIndices.enc = indexOf(parts, QLatin1String("enc"));
                m_dmonIndices.dec = indexOf(parts, QLatin1String("dec"));
                m_dmonIndices.fb = indexOf(parts, QLatin1String("fb"));
                m_dmonIndices.bar1 = indexOf(parts, QLatin1String("bar1"));
                m_dmonIndices.mclk = indexOf(parts, QLatin1String("mclk"));
                m_dmonIndices.pclk = indexOf(parts, QLatin1String("pclk"));
            }
            continue;
        }

        bool ok;
        int index = parts[0].toInt(&ok);
        if (!ok) {
            continue;
        }

        auto readDataIfFound =  [&parts, this] (int index) {
            return index >= 0 ? parts[index].toUInt() : 0;
        };

        GpuData data;
        data.index = readDataIfFound(m_dmonIndices.gpu);
        data.power = readDataIfFound(m_dmonIndices.power);
        data.temperature = readDataIfFound(m_dmonIndices.gtemp);

        // GPU usage equals "SM" usage + "ENC" usage + "DEC" usage
        data.usage = readDataIfFound(m_dmonIndices.sm) + readDataIfFound(m_dmonIndices.enc) + readDataIfFound(m_dmonIndices.dec);

        // Total memory used equals "FB" usage + "BAR1" usage
        data.memoryUsed = readDataIfFound(m_dmonIndices.fb) + readDataIfFound(m_dmonIndices.bar1);

        data.memoryFrequency = readDataIfFound(m_dmonIndices.mclk);
        data.coreFrequency = readDataIfFound(m_dmonIndices.pclk);

        Q_EMIT dataReceived(data);
    }
}
