/*
    SPDX-FileCopyrightText: 2019 David Edmundson <davidedmundson@kde.org>
    SPDX-FileCopyrightText: 2020 David Redondo <kde@david-redondo.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "daemon.h"

#include <chrono>

#ifdef HAVE_SENSORS
#include <sensors/sensors.h>
#endif

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusServiceWatcher>

#include <QTimer>

#include <systemstats/SensorPlugin.h>
#include <systemstats/SensorObject.h>
#include <systemstats/SensorContainer.h>
#include <systemstats/SensorProperty.h>

#include <KDBusService>
#include <KPluginLoader>
#include <KPluginMetaData>
#include <KPluginFactory>

#include "ksystemstatsadaptor.h"

#include "client.h"

constexpr auto UpdateRate = std::chrono::milliseconds{500};

Daemon::Daemon()
    : m_serviceWatcher(new QDBusServiceWatcher(this))
{
    qDBusRegisterMetaType<KSysGuard::SensorData>();
    qDBusRegisterMetaType<KSysGuard::SensorInfo>();
    qRegisterMetaType<KSysGuard::SensorDataList>("SDL");
    qDBusRegisterMetaType<KSysGuard::SensorDataList>();
    qDBusRegisterMetaType<KSysGuard::SensorInfoMap>();
    qDBusRegisterMetaType<QStringList>();

    new KsystemstatsAdaptor(this);

    m_serviceWatcher->setConnection(QDBusConnection::sessionBus());
    connect(m_serviceWatcher, &QDBusServiceWatcher::serviceUnregistered, this, &Daemon::onServiceDisconnected);

    auto timer = new QTimer(this);
    timer->setInterval(UpdateRate);
    connect(timer, &QTimer::timeout, this, &Daemon::sendFrame);
    timer->start();
}

Daemon::~Daemon()
{
    for (Client* c : m_clients) {
        delete c;
    }
#ifdef HAVE_SENSORS
    sensors_cleanup();
#endif
}

void Daemon::init(ReplaceIfRunning replaceIfRunning)
{
#ifdef HAVE_SENSORS
    sensors_init(nullptr);
#endif
    loadProviders();
    KDBusService::StartupOptions options = KDBusService::Unique;
    if (replaceIfRunning == ReplaceIfRunning::Replace) {
        options |= KDBusService::Replace;
    }
    QDBusConnection::sessionBus().registerObject("/", this, QDBusConnection::ExportAdaptors);
    auto service = new KDBusService(options , this);
    service->setExitValue(1);
}

void Daemon::setQuitOnLastClientDisconnect(bool quit)
{
    m_quitOnLastClientDisconnect = quit;
}

void Daemon::loadProviders()
{
    const QVector<KPluginMetaData> plugins = KPluginLoader::findPlugins(QStringLiteral("ksystemstats"));
    if (plugins.isEmpty()) {
        qWarning() << "No plugins found";
    }

    for (const KPluginMetaData &metaData : plugins) {
        KPluginLoader pluginLoader(metaData.fileName());
        KSysGuard::SensorPlugin *provider = nullptr;
        if (KPluginFactory *factory = pluginLoader.factory()) {
            provider = factory->create<KSysGuard::SensorPlugin>(this);
            if (provider) {
                registerProvider(provider);
            }
        }
        if (!provider) {
            qWarning() << "Could not load plugin:" << metaData.pluginId() << "with file name" << metaData.fileName();
        }
    }
}

void Daemon::registerProvider(KSysGuard::SensorPlugin *provider) {
    m_providers.append(provider);
    const auto containers = provider->containers();
    for (auto container : containers) {
        m_containers[container->id()] = container;
        connect(container, &KSysGuard::SensorContainer::objectAdded, this, [this](KSysGuard::SensorObject *obj) {
            for (auto sensor: obj->sensors()) {
                emit sensorAdded(sensor->path());
            }
        });
        connect(container, &KSysGuard::SensorContainer::objectRemoved, this, [this](KSysGuard::SensorObject *obj) {
            for (auto sensor: obj->sensors()) {
                emit sensorRemoved(sensor->path());
            }
        });
    }
}

KSysGuard::SensorInfoMap Daemon::allSensors() const
{
    KSysGuard::SensorInfoMap infoMap;
    for (auto c : qAsConst(m_containers)) {
        auto containerInfo = KSysGuard::SensorInfo{};
        containerInfo.name = c->name();
        infoMap.insert(c->id(), containerInfo);

        const auto objects = c->objects();
        for(auto object : objects) {
            auto objectInfo = KSysGuard::SensorInfo{};
            objectInfo.name = object->name();
            infoMap.insert(object->path(), objectInfo);

            const auto sensors = object->sensors();
            for (auto sensor : sensors) {
                infoMap[sensor->path()] = sensor->info();
            }
        }
    }
    return infoMap;
}

KSysGuard::SensorInfoMap Daemon::sensors(const QStringList &sensorPaths) const
{
    KSysGuard::SensorInfoMap si;
    for (const QString &path : sensorPaths) {
        if (auto sensor = findSensor(path)) {
            si[path] = sensor->info();
        }
    }
    return si;
}

void Daemon::subscribe(const QStringList &sensorIds)
{
    const QString sender = QDBusContext::message().service();
    m_serviceWatcher->addWatchedService(sender);

    Client *client = m_clients.value(sender);
    if (!client) {
        client = new Client(this, sender);
        m_clients[sender] = client;
    }
    client->subscribeSensors(sensorIds);
}

void Daemon::unsubscribe(const QStringList &sensorIds)
{
    const QString sender = QDBusContext::message().service();
    Client *client = m_clients.value(sender);
    if (!client) {
        return;
    }
    client->unsubscribeSensors(sensorIds);
}

KSysGuard::SensorDataList Daemon::sensorData(const QStringList &sensorIds)
{
    KSysGuard::SensorDataList sensorData;
    for (const QString &sensorId: sensorIds) {
        if (KSysGuard::SensorProperty *sensorProperty = findSensor(sensorId)) {
            const QVariant value = sensorProperty->value();
            if (value.isValid()) {
                sensorData << KSysGuard::SensorData(sensorId, value);
            }
        }
    }
    return sensorData;
}

KSysGuard::SensorProperty *Daemon::findSensor(const QString &path) const
{
    int subsystemIndex = path.indexOf('/');
    int propertyIndex = path.lastIndexOf('/');

    const QString subsystem = path.left(subsystemIndex);
    const QString object = path.mid(subsystemIndex + 1, propertyIndex - (subsystemIndex + 1));
    const QString property = path.mid(propertyIndex + 1);

    auto c = m_containers.value(subsystem);
    if (!c) {
        return nullptr;
    }
    auto o = c->object(object);
    if (!o) {
        return nullptr;
    }
    return o->sensor(property);
}

void Daemon::onServiceDisconnected(const QString &service)
{
    delete m_clients.take(service);
    if (m_clients.isEmpty() && m_quitOnLastClientDisconnect) {
        QCoreApplication::quit();
    };
}

void Daemon::sendFrame()
{
    for (auto provider : qAsConst(m_providers)) {
        provider->update();
    }

    for (auto client: qAsConst(m_clients)) {
        client->sendFrame();
    }
}
