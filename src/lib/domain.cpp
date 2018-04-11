/*
 * Copyright (C) 2018 Daniel Nicoletti <dantti12@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include "domain.h"
#include "connection.h"

#include "virtlyst.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(VIRT_DOM, "virt.domain")

Domain::Domain(virDomainPtr domain, Connection *conn, QObject *parent) : QObject(parent)
  , m_conn(conn)
  , m_domain(domain)
{

}

Domain::~Domain()
{
    virDomainFree(m_domain);
}

bool Domain::saveXml()
{
    qCCritical(VIRT_DOM) << xmlDoc().toString();
    return m_conn->domainDefineXml(xmlDoc().toString(0));
}

QString Domain::xml()
{
    return xmlDoc().toString(2);
}

QString Domain::name() const
{
    const char *name = virDomainGetName(m_domain);
    if (!name) {
        qCWarning(VIRT_DOM) << "Failed to get domain name";
        return QString();
    }
    return QString::fromUtf8(name);
}

QString Domain::uuid()
{
    return dataFromSimpleNode(QStringLiteral("uuid"));
}

QString Domain::title()
{
    return dataFromSimpleNode(QStringLiteral("title"));
}

void Domain::setTitle(const QString &title)
{
    setDataToSimpleNode(QStringLiteral("title"), title);
}

QString Domain::description()
{
    return dataFromSimpleNode(QStringLiteral("description"));
}

void Domain::setDescription(const QString &description)
{
    setDataToSimpleNode(QStringLiteral("description"), description);
}

int Domain::status()
{
    if (!m_gotInfo) {
        if (virDomainGetInfo(m_domain, &m_info) < 0) {
            qCWarning(VIRT_DOM) << "Failed to get info for domain" << name();
            return -1;
        }
        m_gotInfo = true;
    }
    return m_info.state;
}

int Domain::currentVcpu()
{
    QDomElement docElem = xmlDoc().documentElement();
    QDomElement node = docElem.firstChildElement(QStringLiteral("vcpu"));
    if (node.hasAttribute(QStringLiteral("current"))) {
        return node.attribute(QStringLiteral("current")).toInt();
    }
    QDomNode data = node.firstChild();
    return data.nodeValue().toInt();
}

void Domain::setCurrentVcpu(int number)
{
    QDomElement docElem = xmlDoc().documentElement();
    QDomElement node = docElem.firstChildElement(QStringLiteral("vcpu"));
    node.setAttribute(QStringLiteral("current"), number);
}

int Domain::vcpu()
{
    return dataFromSimpleNode(QStringLiteral("vcpu")).toULongLong();
}

void Domain::setVcpu(int number)
{
    setDataToSimpleNode(QStringLiteral("vcpu"), QString::number(number));
}

quint64 Domain::memory()
{
    return dataFromSimpleNode(QStringLiteral("memory")).toULongLong();
}

void Domain::setMemory(quint64 kBytes)
{
    setDataToSimpleNode(QStringLiteral("memory"), QString::number(kBytes));
}

int Domain::memoryMiB()
{
    return memory() / 1024;
}

quint64 Domain::currentMemory()
{
    return dataFromSimpleNode(QStringLiteral("currentMemory")).toULongLong();
}

void Domain::setCurrentMemory(quint64 kBytes)
{
    qCritical() << Q_FUNC_INFO << QString::number(kBytes) << kBytes;
    setDataToSimpleNode(QStringLiteral("currentMemory"), QString::number(kBytes));
}

int Domain::currentMemoryMiB()
{
    return currentMemory() / 1024;
}

QString Domain::currentMemoryPretty()
{
    return Virtlyst::prettyKibiBytes(currentMemory());
}

bool Domain::hasManagedSaveImage() const
{
    return virDomainHasManagedSaveImage(m_domain, 0);
}

bool Domain::autostart() const
{
    int autostart = 0;
    if (virDomainGetAutostart(m_domain, &autostart) < 0) {
        qWarning() << "Failed to get autostart for domain" << name();
    }
    return autostart;
}

QString Domain::consoleType()
{
    return xmlDoc().documentElement()
            .firstChildElement(QStringLiteral("devices"))
            .firstChildElement(QStringLiteral("graphics"))
            .attribute(QStringLiteral("type"));
}

void Domain::setConsoleType(const QString &type)
{
    xmlDoc().documentElement()
            .firstChildElement(QStringLiteral("devices"))
            .firstChildElement(QStringLiteral("graphics"))
            .setAttribute(QStringLiteral("type"), type);
}

QString Domain::consolePassword()
{
    return xmlDoc().documentElement()
            .firstChildElement(QStringLiteral("devices"))
            .firstChildElement(QStringLiteral("graphics"))
            .attribute(QStringLiteral("passwd"));
}

void Domain::setConsolePassword(const QString &password)
{
    xmlDoc().documentElement()
            .firstChildElement(QStringLiteral("devices"))
            .firstChildElement(QStringLiteral("graphics"))
            .setAttribute(QStringLiteral("passwd"), password);
}

quint32 Domain::consolePort()
{
    return xmlDoc().documentElement()
            .firstChildElement(QStringLiteral("devices"))
            .firstChildElement(QStringLiteral("graphics"))
            .attribute(QStringLiteral("port")).toUInt();
}

QString Domain::consoleListenAddress()
{
    QString ret = xmlDoc().documentElement()
                .firstChildElement(QStringLiteral("devices"))
                .firstChildElement(QStringLiteral("graphics"))
                .attribute(QStringLiteral("listen"));
    if (ret.isEmpty()) {
        ret = xmlDoc().documentElement()
                .firstChildElement(QStringLiteral("devices"))
                .firstChildElement(QStringLiteral("graphics"))
                .firstChildElement(QStringLiteral("listen"))
                .attribute(QStringLiteral("address"));
        if (ret.isEmpty()) {
            return QStringLiteral("127.0.0.1");
        }
    }
    return ret;
}

QString Domain::consoleKeymap()
{
    return xmlDoc().documentElement()
            .firstChildElement(QStringLiteral("devices"))
            .firstChildElement(QStringLiteral("graphics"))
            .attribute(QStringLiteral("keymap"));
}

void Domain::start()
{
    virDomainCreate(m_domain);
}

void Domain::shutdown()
{
    virDomainShutdown(m_domain);
}

void Domain::suspend()
{
    virDomainSuspend(m_domain);
}

void Domain::resume()
{
    virDomainResume(m_domain);
}

void Domain::destroy()
{
    virDomainDestroy(m_domain);
}

void Domain::undefine()
{
    virDomainUndefine(m_domain);
}

void Domain::managedSave()
{
    virDomainManagedSave(m_domain, 0);
}

void Domain::managedSaveRemove()
{
    virDomainManagedSaveRemove(m_domain, 0);
}

void Domain::setAutostart(bool enable)
{
    virDomainSetAutostart(m_domain, enable ? 1 : 0);
}

QDomDocument Domain::xmlDoc()
{
    if (m_xml.isNull()) {
        char *xml = virDomainGetXMLDesc(m_domain, VIR_DOMAIN_XML_SECURE);
        const QString xmlString = QString::fromUtf8(xml);
//        qDebug() << "XML" << xml;
        QString error;
        if (!m_xml.setContent(xmlString, &error)) {
            qWarning() << "Failed to parse XML from interface" << error;
        }
        free(xml);
    }
    return m_xml;
}

QString Domain::dataFromSimpleNode(const QString &element)
{
    return xmlDoc().documentElement()
            .firstChildElement(element)
            .firstChild()
            .nodeValue();
}

void Domain::setDataToSimpleNode(const QString &element, const QString &data)
{
    xmlDoc().documentElement()
            .firstChildElement(element)
            .firstChild()
            .setNodeValue(data);
}
