/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2022, nymea GmbH
* Contact: contact@nymea.io
*
* This file is part of nymea.
* This project including source code and documentation is protected by
* copyright law, and remains the property of nymea GmbH. All rights, including
* reproduction, publication, editing and translation, are reserved. The use of
* this project is subject to the terms of a license agreement to be concluded
* with nymea GmbH in accordance with the terms of use of nymea GmbH, available
* under https://nymea.io/license
*
* GNU Lesser General Public License Usage
* Alternatively, this project may be redistributed and/or modified under the
* terms of the GNU Lesser General Public License as published by the Free
* Software Foundation; version 3. This project is distributed in the hope that
* it will be useful, but WITHOUT ANY WARRANTY; without even the implied
* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this project. If not, see <https://www.gnu.org/licenses/>.
*
* For any further details and any questions please contact us under
* contact@nymea.io or see our FAQ/Licensing Information on
* https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "zwaveintegrationplugin.h"

#include <hardware/zwave/zwavehardwareresource.h>


ZWaveIntegrationPlugin::ZWaveIntegrationPlugin(ZWaveHardwareResource::HandlerType handlerType, const QLoggingCategory &loggingCategory):
    m_handlerType(handlerType),
    m_dc(loggingCategory.categoryName())
{

}

ZWaveIntegrationPlugin::~ZWaveIntegrationPlugin()
{

}

void ZWaveIntegrationPlugin::init()
{
    hardwareManager()->zwaveResource()->registerHandler(this, m_handlerType);
}

void ZWaveIntegrationPlugin::handleRemoveNode(ZWaveNode *node)
{
    foreach (Thing *thing, m_thingNodes.keys(node)) {
        emit autoThingDisappeared(thing->id());

        // Removing it from our map to prevent a loop that would ask the zigbee network to remove this node (see thingRemoved())
        m_thingNodes.remove(thing);
    }
}

void ZWaveIntegrationPlugin::thingRemoved(Thing *thing)
{
    m_thingNodes.remove(thing);
}

bool ZWaveIntegrationPlugin::manageNode(Thing *thing)
{
    Q_UNUSED(thing)
    QUuid networkUuid = thing->paramValue(thing->thingClass().paramTypes().findByName("networkUuid").id()).toUuid();
    quint8 nodeId = thing->paramValue(thing->thingClass().paramTypes().findByName("nodeId").id()).toUInt();

    ZWaveNode *node = m_thingNodes.value(thing);
    if (!node) {
        node = hardwareManager()->zwaveResource()->claimNode(this, networkUuid, nodeId);
    }

    if (!node) {
        return false;
    }

    m_thingNodes.insert(thing, node);

    // Update connected state
    thing->setStateValue("connected", node->reachable());
    connect(node, &ZWaveNode::reachableChanged, thing, [thing](bool reachable){
        thing->setStateValue("connected", reachable);
    });

    // Update signal strength
    thing->setStateValue("signalStrength", node->linkQuality());
    connect(node, &ZWaveNode::linkQualityChanged, thing, [thing](quint8 linkQuality){
        thing->setStateValue("signalStrength", linkQuality);
    });

    return true;
}

Thing *ZWaveIntegrationPlugin::thingForNode(ZWaveNode *node)
{
    return m_thingNodes.key(node);
}

ZWaveNode *ZWaveIntegrationPlugin::nodeForThing(Thing *thing)
{
    return m_thingNodes.value(thing);
}

void ZWaveIntegrationPlugin::createThing(const ThingClassId &thingClassId, ZWaveNode *node, const ParamList &additionalParams)
{
    Q_UNUSED(node)
    ThingDescriptor descriptor(thingClassId, node->productName());
    QString deviceClassName = supportedThings().findById(thingClassId).displayName();

    ParamList params;
    ThingClass tc = supportedThings().findById(thingClassId);
    params.append(Param(tc.paramTypes().findByName("networkUuid").id(), node->networkUuid().toString()));
    params.append(Param(tc.paramTypes().findByName("nodeId").id(), node->nodeId()));
    params.append(additionalParams);
    descriptor.setParams(params);
    emit autoThingsAppeared({descriptor});
}

