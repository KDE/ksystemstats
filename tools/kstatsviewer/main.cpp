/*
    SPDX-FileCopyrightText: 2020 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include <QCoreApplication>
#include <QDebug>

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusContext>
#include <QDBusMessage>
#include <QDBusMetaType>

#include <QCommandLineOption>
#include <QCommandLineParser>

#include <iostream>

#include <formatter/Formatter.h>
#include <systemstats/DBusInterface.h>

class SensorWatcher : public QCoreApplication
{
    Q_OBJECT

public:
    SensorWatcher(int &argc, char **argv);
    ~SensorWatcher() = default;

    void subscribe(const QStringList &sensorNames);
    void list();
    void showDetails(const QStringList &sensorNames);

    void setShowDetails(bool details);

private:
    void onNewSensorData(const KSysGuard::SensorDataList &changes);
    void onSensorMetaDataChanged(const KSysGuard::SensorInfoMap &sensors);
    void showSensorDetails(const KSysGuard::SensorInfoMap &sensors);
    KSysGuard::SystemStats::DBusInterface *m_iface;
    bool m_showDetails = false;
};

int main(int argc, char **argv)
{
    qDBusRegisterMetaType<KSysGuard::SensorData>();
    qDBusRegisterMetaType<KSysGuard::SensorInfo>();
    qDBusRegisterMetaType<KSysGuard::SensorDataList>();
    qDBusRegisterMetaType<QHash<QString, KSysGuard::SensorInfo>>();
    qDBusRegisterMetaType<QStringList>();

    SensorWatcher app(argc, argv);

    QCommandLineParser parser;
    parser.addOption({ QStringLiteral("list"), QStringLiteral("List Available Sensors") });
    parser.addOption({ QStringLiteral("details"), QStringLiteral("Show detailed information about selected sensors") });

    parser.addPositionalArgument(QStringLiteral("sensorNames"), QStringLiteral("List of sensors to monitor"), QStringLiteral("sensorId1 sensorId2  ..."));
    parser.addHelpOption();
    parser.process(app);

    if (parser.isSet(QStringLiteral("list"))) {
        app.list();
    } else if (parser.positionalArguments().isEmpty()) {
        qDebug() << "No sensors specified.";
        parser.showHelp(-1);
    } else {
        app.setShowDetails(parser.isSet(QStringLiteral("details")));
        app.subscribe(parser.positionalArguments());
        app.exec();
    }
}

SensorWatcher::SensorWatcher(int &argc, char **argv)
    : QCoreApplication(argc, argv)
    , m_iface(new KSysGuard::SystemStats::DBusInterface("org.kde.ksystemstats",
          "/",
          QDBusConnection::sessionBus(),
          this))
{
    connect(m_iface, &KSysGuard::SystemStats::DBusInterface::newSensorData, this, &SensorWatcher::onNewSensorData);
    connect(m_iface, &KSysGuard::SystemStats::DBusInterface::sensorMetaDataChanged, this, &SensorWatcher::onSensorMetaDataChanged);
}

namespace
{
/** Complain about sensor-name mismatches
 *
 * Check that all the @p sensorNames have data; those that do not are
 * **possibly** typo's in the sensor names, so warn the user about that.
 */
void showMissingSensors(const KSysGuard::SensorDataList &data, const QStringList& sensorNames)
{
    QSet<QString> setNames(sensorNames.begin(), sensorNames.end());
    for (const auto &sensor : qAsConst(data)) {
        const QString name = sensor.sensorProperty;
        if (setNames.contains(name)) {
            setNames.remove(name);
        } else {
            std::cout << "Unexpected data for sensor '" << qPrintable(name) << "'\n";
        }
    }
    for (const auto& name : setNames) {
        std::cout << "No data for sensor '" << qPrintable(name) << "'\n";
    }
}
}

void SensorWatcher::subscribe(const QStringList &sensorNames)
{
    m_iface->subscribe(sensorNames);

    auto pendingInitialValues = m_iface->sensorData(sensorNames);
    pendingInitialValues.waitForFinished();
    {
        auto sensorValues = pendingInitialValues.value();
        showMissingSensors(sensorValues, sensorNames);
        onNewSensorData(sensorValues);
    }

    if (m_showDetails) {
        auto pendingSensors = m_iface->sensors(sensorNames);
        pendingSensors.waitForFinished();

        auto sensors = pendingSensors.value();
        showSensorDetails(sensors);
    }
}

void SensorWatcher::onNewSensorData(const KSysGuard::SensorDataList &changes)
{
    for (const auto &entry : changes) {
        std::cout << qPrintable(entry.sensorProperty) << ' ' << qPrintable(entry.payload.toString()) << std::endl;
    }
}

void SensorWatcher::onSensorMetaDataChanged(const KSysGuard::SensorInfoMap &sensors)
{
    if (!m_showDetails) {
        return;
    }

    std::cout << "Sensor metadata changed\n";
    showSensorDetails(sensors);
}

void SensorWatcher::list()
{
    auto pendingSensors = m_iface->allSensors();
    pendingSensors.waitForFinished();
    auto sensors = pendingSensors.value();
    if (sensors.count() < 1) {
        std::cout << "No sensors available.";
    }
    for (auto it = sensors.constBegin(); it != sensors.constEnd(); it++) {
        std::cout << qPrintable(it.key()) << ' ' << qPrintable(it.value().name) << std::endl;
    }
}

void SensorWatcher::setShowDetails(bool details)
{
    m_showDetails = details;
}

void SensorWatcher::showSensorDetails(const KSysGuard::SensorInfoMap &sensors)
{
    for (auto it = sensors.constBegin(); it != sensors.constEnd(); ++it) {
        auto info = it.value();
        std::cout << qPrintable(it.key()) << "\n";
        std::cout << "    Name:        " << qPrintable(info.name) << "\n";
        std::cout << "    Short Name:  " << qPrintable(info.shortName) << "\n";
        std::cout << "    Description: " << qPrintable(info.description) << "\n";
        std::cout << "    Unit:        " << qPrintable(KSysGuard::Formatter::symbol(info.unit)) << "\n";
        std::cout << "    Minimum:     " << info.min << "\n";
        std::cout << "    Maximum:     " << info.max << "\n";
    }
}

#include "main.moc"
