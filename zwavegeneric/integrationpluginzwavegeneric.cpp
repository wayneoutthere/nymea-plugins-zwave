/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
*
* Copyright 2013 - 2022, nymea GmbH
* Contact: contact@nymea.io

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

#include "integrationpluginzwavegeneric.h"
#include "plugininfo.h"
#include "hardware/zwave/zwavehardwareresource.h"

#include <QDebug>

IntegrationPluginZWaveGeneric::IntegrationPluginZWaveGeneric(): ZWaveIntegrationPlugin(ZWaveHardwareResource::HandlerTypeCatchAll, dcZWaveGeneric())
{
}

QString IntegrationPluginZWaveGeneric::name() const
{
    return "Generic";
}

bool IntegrationPluginZWaveGeneric::handleNode(ZWaveNode *node)
{
    qCDebug(dcZWaveGeneric()) << "Handle node for node:" << node;

    // power socket
    if (node->value(ZWaveValue::GenreUser, ZWaveValue::CommandClassSwitchBinary, 1, 0, ZWaveValue::TypeBool).isValid()) {
        qCDebug(dcZWaveGeneric()) << "Device is a binary switch";

        if (node->value(ZWaveValue::GenreUser, ZWaveValue::CommandClassMeter, 1, 0, ZWaveValue::TypeDecimal).isValid()) {
            createThing(powerMeterSocketThingClassId, node, {
                            {powerMeterSocketThingManufacturerParamTypeId, node->manufacturerName()},
                            {powerMeterSocketThingModelParamTypeId, node->productName()}
                        });
        } else {
            createThing(powerSocketThingClassId, node, {
                            {powerSocketThingManufacturerParamTypeId, node->manufacturerName()},
                            {powerSocketThingModelParamTypeId, node->productName()}
                        });
        }
        return true;
    }

    if (node->value(ZWaveValue::GenreUser, ZWaveValue::CommandClassDoorLock, 1, 0, ZWaveValue::TypeBool).isValid()) {
        qCDebug(dcZWaveGeneric()) << "Device is a door lock";

        createThing(doorLockThingClassId, node, {
                        {doorLockThingManufacturerParamTypeId, node->manufacturerName()},
                        {doorLockThingModelParamTypeId, node->productName()}
                    });

        return true;
    }

    return false;
}

void IntegrationPluginZWaveGeneric::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    if (!manageNode(thing)) {
        qCWarning(dcZWaveGeneric()) << "Failed to claim node during setup.";
        info->finish(Thing::ThingErrorHardwareNotAvailable);
        return;
    }

    ZWaveNode* node = nodeForThing(thing);
    if (!node) {
        qCWarning(dcZWaveGeneric()) << "Could not find ZWave node for" << thing;
        info->finish(Thing::ThingErrorSetupFailed);
        return;
    }

    if (thing->thingClassId() == powerSocketThingClassId) {

        info->finish(Thing::ThingErrorNoError);

        thing->setStateValue(powerSocketPowerStateTypeId, node->value(ZWaveValue::GenreUser, ZWaveValue::CommandClassSwitchBinary, 1, 0, ZWaveValue::TypeBool).value().toBool());

        connect(node, &ZWaveNode::valueChanged, thing, [thing](const ZWaveValue &value){
            if (value.genre() == ZWaveValue::GenreUser && value.commandClass() == ZWaveValue::CommandClassSwitchBinary) {
                thing->setStateValue(powerSocketPowerStateTypeId, value.value().toBool());
            }
        });
        return;
    }

    if (thing->thingClassId() == powerMeterSocketThingClassId) {

        thing->setStateValue(powerMeterSocketPowerStateTypeId, node->value(ZWaveValue::GenreUser, ZWaveValue::CommandClassSwitchBinary, 1, 0, ZWaveValue::TypeBool).value().toBool());
        info->finish(Thing::ThingErrorNoError);

        thing->setStateValue(powerMeterSocketCurrentPowerStateTypeId, node->value(ZWaveValue::GenreUser, ZWaveValue::CommandClassSensorMultilevel, 1, 4, ZWaveValue::TypeDecimal).value().toDouble());
        thing->setStateValue(powerMeterSocketTotalEnergyConsumedStateTypeId, node->value(ZWaveValue::GenreUser, ZWaveValue::CommandClassMeter, 1, 0, ZWaveValue::TypeDecimal).value().toDouble());
        connect(node, &ZWaveNode::valueChanged, thing, [thing](const ZWaveValue &value){
            if (value.genre() == ZWaveValue::GenreUser && value.commandClass() == ZWaveValue::CommandClassSwitchBinary) {
                thing->setStateValue(powerMeterSocketPowerStateTypeId, value.value().toBool());
            }
            if (value.genre() == ZWaveValue::GenreUser && value.commandClass() == ZWaveValue::CommandClassSensorMultilevel) {
                thing->setStateValue(powerMeterSocketCurrentPowerStateTypeId, value.value().toDouble());
            } if (value.genre() == ZWaveValue::GenreUser && value.commandClass() == ZWaveValue::CommandClassMeter && value.index() == 0) {
                thing->setStateValue(powerMeterSocketTotalEnergyConsumedStateTypeId, value.value().toDouble());
            }
        });
        return;
    }

    if (thing->thingClassId() == doorLockThingClassId) {

        thing->setStateValue(doorLockStateStateTypeId, node->value(ZWaveValue::GenreUser, ZWaveValue::CommandClassDoorLock, 1, 0, ZWaveValue::TypeBool).value().toBool() ? "locked" : "unlocked");
        info->finish(Thing::ThingErrorNoError);

        connect(node, &ZWaveNode::valueChanged, thing, [thing](const ZWaveValue &value){
            if (value.genre() == ZWaveValue::GenreUser && value.commandClass() == ZWaveValue::CommandClassDoorLock && value.index() == 0) {
                thing->setStateValue(doorLockStateStateTypeId, value.value().toBool() ? "locked" : "unlocked");
            }
        });
        return;
    }

    info->finish(Thing::ThingErrorUnsupportedFeature);
}

void IntegrationPluginZWaveGeneric::executeAction(ThingActionInfo *info)
{
    if (!hardwareManager()->zwaveResource()->available()) {
        info->finish(Thing::ThingErrorHardwareNotAvailable);
        return;
    }

    Thing *thing = info->thing();
    ZWaveNode *node = nodeForThing(info->thing());
    if (!node->reachable()) {
        info->finish(Thing::ThingErrorHardwareNotAvailable);
        return;
    }

    if (thing->thingClassId() == powerSocketThingClassId) {
        if (info->action().actionTypeId() == powerSocketPowerActionTypeId) {
            ZWaveValue powerValue = node->value(ZWaveValue::GenreUser, ZWaveValue::CommandClassSwitchBinary, 1, 0, ZWaveValue::TypeBool);
            powerValue.setValue(info->action().paramValue(powerSocketPowerActionPowerParamTypeId).toBool());
            node->setValue(powerValue);
            thing->setStateValue(powerSocketPowerStateTypeId, info->action().paramValue(powerSocketPowerActionPowerParamTypeId));
            info->finish(Thing::ThingErrorNoError);
            return;
        }
    }

    if (thing->thingClassId() == powerMeterSocketThingClassId) {
        if (info->action().actionTypeId() == powerMeterSocketPowerActionTypeId) {
            ZWaveValue powerValue = node->value(ZWaveValue::GenreUser, ZWaveValue::CommandClassSwitchBinary, 1, 0, ZWaveValue::TypeBool);
            powerValue.setValue(info->action().paramValue(powerMeterSocketPowerActionPowerParamTypeId).toBool());
            node->setValue(powerValue);
            thing->setStateValue(powerMeterSocketPowerStateTypeId, info->action().paramValue(powerMeterSocketPowerActionPowerParamTypeId));
            info->finish(Thing::ThingErrorNoError);
            return;
        }
    }

    if (thing->thingClassId() == doorLockThingClassId) {
        if (info->action().actionTypeId() == doorLockLockActionTypeId) {
            ZWaveValue lockValue = node->value(ZWaveValue::GenreUser, ZWaveValue::CommandClassDoorLock, 1, 0, ZWaveValue::TypeBool);
            lockValue.setValue(false);
            node->setValue(lockValue);
//            thing->setStateValue(doorLockStateStateTypeId, "locked");
            info->finish(Thing::ThingErrorNoError);
            return;
        }
        if (info->action().actionTypeId() == doorLockUnlockActionTypeId) {
            ZWaveValue lockValue = node->value(ZWaveValue::GenreUser, ZWaveValue::CommandClassDoorLock, 1, 0, ZWaveValue::TypeBool);
            lockValue.setValue(true);
            node->setValue(lockValue);
//            thing->setStateValue(doorLockStateStateTypeId, "unlocked");
            info->finish(Thing::ThingErrorNoError);
            return;
        }
    }

    info->finish(Thing::ThingErrorUnsupportedFeature);
}

