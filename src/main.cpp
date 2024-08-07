/*
    SPDX-FileCopyrightText: 2019 David Edmundson <davidedmundson@kde.org>
    SPDX-FileCopyrightText: 2020 David Redondo <kde@david-redondo.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include <QCommandLineParser>
#include <QCoreApplication>

#include <KAboutData>
#include <KCrash>

#include "daemon.h"
#include "version.h"

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    app.setQuitLockEnabled(false) ;

    KAboutData about(QStringLiteral("ksystemstats"), QString(), QStringLiteral(KSYSTEMSTATS_VERSION_STRING));
    KAboutData::setApplicationData(about);

    KCrash::initialize();

    QCommandLineParser parser;
    parser.addOption(QCommandLineOption(QStringLiteral("replace"), QStringLiteral("Replace the running instance")));
    parser.addOption({QStringLiteral("remain"), QStringLiteral("Do not quit when last client has disconnected")});
    parser.addHelpOption();
    parser.process(app);

    Daemon d;
    if (!d.init(parser.isSet(QStringLiteral("replace")) ? Daemon::ReplaceIfRunning::Replace : Daemon::ReplaceIfRunning::DoNotReplace)) {
        return 1;
    }

    d.setQuitOnLastClientDisconnect(!parser.isSet(QStringLiteral("remain")));
    return app.exec();
}
