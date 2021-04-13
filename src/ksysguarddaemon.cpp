/*
    Copyright (c) 2019 David Edmundson <davidedmundson@kde.org>
    Copyright (c) 2020 David Redondo <kde@david-redondo.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "ksysguarddaemon.h"

#include <chrono>

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

KSysGuardDaemon::KSysGuardDaemon()
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
    connect(m_serviceWatcher, &QDBusServiceWatcher::serviceUnregistered, this, &KSysGuardDaemon::onServiceDisconnected);

    auto timer = new QTimer(this);
    timer->setInterval(UpdateRate);
    connect(timer, &QTimer::timeout, this, &KSysGuardDaemon::sendFrame);
    timer->start();
}

KSysGuardDaemon::~KSysGuardDaemon()
{
    for (Client* c : m_clients) {
        delete c;
    }
}

void KSysGuardDaemon::init(ReplaceIfRunning replaceIfRunning)
{
    loadProviders();
    KDBusService::StartupOptions options = KDBusService::Unique;
    if (replaceIfRunning == ReplaceIfRunning::Replace) {
        options |= KDBusService::Replace;
    }
    QDBusConnection::sessionBus().registerObject("/", this, QDBusConnection::ExportAdaptors);
    auto service = new KDBusService(options , this);
    service->setExitValue(1);
}

void KSysGuardDaemon::loadProviders()
{
    //instantiate all plugins
    QSet<QString> knownPlugins;
    std::for_each(m_providers.cbegin(), m_providers.cend(), [&knownPlugins] (const KSysGuard::SensorPlugin *plugin) {
        knownPlugins.insert(plugin->providerName());
    });
    const auto plugins = KPluginLoader::instantiatePlugins(QStringLiteral("ksystemstats"), [this, &knownPlugins](const KPluginMetaData &metaData) {
        auto providerName = metaData.rawData().value("providerName").toString();
        if (knownPlugins.contains(providerName)) {
            return false;
        }
        knownPlugins.insert(providerName);
        return true;
    }, this);

    for (auto object : plugins) {
        auto factory = qobject_cast<KPluginFactory*>(object);
        if (!factory) {
            qWarning() << "Plugin object" << object << "did not provide a proper KPluginFactory";
            continue;
        }
        auto provider = factory->create<KSysGuard::SensorPlugin>(this);
        if (!provider) {
            qWarning() << "Plugin object" << object << "did not provide a proper SensorPlugin";
            continue;
        }
        registerProvider(provider);
    }

    if (m_providers.isEmpty()) {
        qWarning() << "No plugins found";
    }
}

void KSysGuardDaemon::registerProvider(KSysGuard::SensorPlugin *provider) {
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

KSysGuard::SensorInfoMap KSysGuardDaemon::allSensors() const
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

KSysGuard::SensorInfoMap KSysGuardDaemon::sensors(const QStringList &sensorPaths) const
{
    KSysGuard::SensorInfoMap si;
    for (const QString &path : sensorPaths) {
        if (auto sensor = findSensor(path)) {
            si[path] = sensor->info();
        }
    }
    return si;
}

void KSysGuardDaemon::subscribe(const QStringList &sensorIds)
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

void KSysGuardDaemon::unsubscribe(const QStringList &sensorIds)
{
    const QString sender = QDBusContext::message().service();
    Client *client = m_clients.value(sender);
    if (!client) {
        return;
    }
    client->unsubscribeSensors(sensorIds);
}

KSysGuard::SensorDataList KSysGuardDaemon::sensorData(const QStringList &sensorIds)
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

KSysGuard::SensorProperty *KSysGuardDaemon::findSensor(const QString &path) const
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

void KSysGuardDaemon::onServiceDisconnected(const QString &service)
{
    delete m_clients.take(service);
    if (m_clients.isEmpty()) {
        QCoreApplication::quit();
    };
}

void KSysGuardDaemon::sendFrame()
{
    for (auto provider : qAsConst(m_providers)) {
        provider->update();
    }

    for (auto client: qAsConst(m_clients)) {
        client->sendFrame();
    }
}
