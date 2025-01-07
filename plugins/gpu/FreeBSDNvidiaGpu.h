/*
 * SPDX-FileCopyrightText: 2024 Henry Hu <henry.hu.sh@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "NvidiaGpu.h"

class FreeBSDNvidiaGpu : public NvidiaGpu
{
    Q_OBJECT

public:

    FreeBSDNvidiaGpu(const QString& id, const QString& name, const QString& location);
};
