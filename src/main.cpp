/*
    SPDX-FileCopyrightText: 2019 David Edmundson <davidedmundson@kde.org>
    SPDX-FileCopyrightText: 2020 David Redondo <kde@david-redondo.de>

    SPDX-License-Identifier: GPL-2.1-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>

#include "ksysguarddaemon.h"

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    app.setQuitLockEnabled(false) ;
    app.setOrganizationDomain(QStringLiteral("kde.org"));

    QCommandLineParser parser;
    parser.addOption(QCommandLineOption(QStringLiteral("replace"), QStringLiteral("Replace the running instance")));
    parser.addOption({QStringLiteral("remain"), QStringLiteral("Do not quit when last client has disconnected")});
    parser.addHelpOption();
    parser.process(app);

    KSysGuardDaemon d;
    d.init(parser.isSet(QStringLiteral("replace")) ? KSysGuardDaemon::ReplaceIfRunning::Replace : KSysGuardDaemon::ReplaceIfRunning::DoNotReplace);
    d.setQuitOnLastClientDisconnect(!parser.isSet(QStringLiteral("remain")));
    app.exec();
}
