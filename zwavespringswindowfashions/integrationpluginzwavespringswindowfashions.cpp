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

#include "integrationpluginzwavespringswindowfashions.h"
#include "plugininfo.h"

#include <hardware/zwave/zwavehardwareresource.h>

#include <QDebug>

IntegrationPluginZWaveSpringsWindowFashions::IntegrationPluginZWaveSpringsWindowFashions(): ZWaveIntegrationPlugin(ZWaveHardwareResource::HandlerTypeVendor, dcZWaveSpringsWindowFashions())
{
}

QString IntegrationPluginZWaveSpringsWindowFashions::name() const
{
    return "Qubino";
}

bool IntegrationPluginZWaveSpringsWindowFashions::handleNode(ZWaveNode *node)
{
    qCDebug(dcZWaveSpringsWindowFashions()) << "Handle node for SpringsWindowFashions" << node;

    if (node->manufacturerId() != 0x026e) {
        return false;
    }

    // RSZ1 Roller Shade
    if (node->productId() == 0x5a31 && node->productType() == 0x5253) {
        createThing(rollerShadeThingClassId, node);
        return true;
    }

    // BRZ1 Remote control
    if (node->productId() == 0x5a31 && node->productType() == 0x4252) {
        createThing(remoteControlThingClassId, node);
        return true;
    }

    return false;
}

void IntegrationPluginZWaveSpringsWindowFashions::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    if (!manageNode(thing)) {
        qCWarning(dcZWaveSpringsWindowFashions()) << "Failed to claim node during setup.";
        info->finish(Thing::ThingErrorHardwareNotAvailable);
        return;
    }

    ZWaveNode *node = nodeForThing(thing);

    if (thing->thingClassId() == rollerShadeThingClassId) {

        info->finish(Thing::ThingErrorNoError);

        thing->setStateValue(rollerShadePercentageStateTypeId, node->value(ZWaveValue::GenreUser, ZWaveValue::CommandClassSwitchMultilevel, 1, 0, ZWaveValue::TypeByte).value().toUInt());
        thing->setStateValue(rollerShadeBatteryLevelStateTypeId, node->value(ZWaveValue::GenreUser, ZWaveValue::CommandClassBattery, 1, 0, ZWaveValue::TypeByte).value().toUInt());
        thing->setStateValue(rollerShadeBatteryCriticalStateTypeId, thing->stateValue(rollerShadeBatteryLevelStateTypeId).toInt() < 5);

        connect(node, &ZWaveNode::valueChanged, thing, [thing](const ZWaveValue &value){
            if (value.genre() == ZWaveValue::GenreConfig && value.commandClass() == ZWaveValue::CommandClassConfiguration && value.index() == 74) {
                thing->setSettingValue(rollerShadeSettingsOpenCloseTimeParamTypeId, value.value());
            }

            if (value.genre() == ZWaveValue::GenreUser && value.commandClass() == ZWaveValue::CommandClassSwitchMultilevel && value.index() == 0) {
                thing->setStateValue(rollerShadePercentageStateTypeId, 100 - value.value().toUInt());
            } else if (value.genre() == ZWaveValue::GenreUser && value.commandClass() == ZWaveValue::CommandClassBattery && value.index() == 0) {
                thing->setStateValue(rollerShadeBatteryLevelStateTypeId, value.value().toUInt());
                thing->setStateValue(rollerShadeBatteryCriticalStateTypeId, value.value().toUInt() < 5);
            }

        });

        return;
    }

    if (info->thing()->thingClassId() == remoteControlThingClassId) {
        thing->setStateValue(remoteControlBatteryLevelStateTypeId, node->value(ZWaveValue::GenreUser, ZWaveValue::CommandClassBattery, 1, 0, ZWaveValue::TypeByte).value().toUInt());
        thing->setStateValue(remoteControlBatteryCriticalStateTypeId, thing->stateValue(rollerShadeBatteryLevelStateTypeId).toInt() < 5);

        info->finish(Thing::ThingErrorNoError);

        connect(node, &ZWaveNode::valueChanged, thing, [thing](const ZWaveValue &value){
            if (value.genre() == ZWaveValue::GenreUser && value.commandClass() == ZWaveValue::CommandClassBattery && value.index() == 0) {
                thing->setStateValue(remoteControlBatteryLevelStateTypeId, value.value().toUInt());
                thing->setStateValue(remoteControlBatteryCriticalStateTypeId, value.value().toUInt() < 5);
            } else if (value.genre() == ZWaveValue::GenreUser && value.commandClass() == ZWaveValue::CommandClassSwitchMultilevel && value.index() == 0) {
                if (value.value().toUInt() <= 1) {
                    thing->emitEvent(remoteControlPressedEventTypeId, ParamList{{remoteControlPressedEventButtonNameParamTypeId, "Down"}});

                } else if (value.value().toUInt() >= 99) {
                    thing->emitEvent(remoteControlPressedEventTypeId, ParamList{{remoteControlPressedEventButtonNameParamTypeId, "Up"}});

                } else {
                    thing->emitEvent(remoteControlPressedEventTypeId, ParamList{{remoteControlPressedEventButtonNameParamTypeId, "Home"}});
                }

            } else if (value.genre() == ZWaveValue::GenreUser && value.commandClass() == ZWaveValue::CommandClassSwitchMultilevel && value.index() == 1) {
                thing->emitEvent(remoteControlPressedEventTypeId, ParamList{{remoteControlPressedEventButtonNameParamTypeId, "Up"}});
            } else if (value.genre() == ZWaveValue::GenreUser && value.commandClass() == ZWaveValue::CommandClassSwitchMultilevel && value.index() == 2) {
                thing->emitEvent(remoteControlPressedEventTypeId, ParamList{{remoteControlPressedEventButtonNameParamTypeId, "Down"}});
            }
        });
        return;
    }
    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginZWaveSpringsWindowFashions::executeAction(ThingActionInfo *info)
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

    if (thing->thingClassId() == rollerShadeThingClassId) {
        if (info->action().actionTypeId() == rollerShadeOpenActionTypeId) {
            ZWaveValue powerValue = node->value(ZWaveValue::GenreUser, ZWaveValue::CommandClassSwitchMultilevel, 1, 1, ZWaveValue::TypeButton);
            powerValue.setValue(true);
            node->setValue(powerValue);
            info->finish(Thing::ThingErrorNoError);
            return;
        }
        if (info->action().actionTypeId() == rollerShadeCloseActionTypeId) {
            ZWaveValue powerValue = node->value(ZWaveValue::GenreUser, ZWaveValue::CommandClassSwitchMultilevel, 1, 2, ZWaveValue::TypeButton);
            powerValue.setValue(true);
            node->setValue(powerValue);
            info->finish(Thing::ThingErrorNoError);
            return;
        }
        if (info->action().actionTypeId() == rollerShadeStopActionTypeId) {
            ZWaveValue openValue = node->value(ZWaveValue::GenreUser, ZWaveValue::CommandClassSwitchMultilevel, 1, 1, ZWaveValue::TypeButton);
            openValue.setValue(false);
            node->setValue(openValue);

            ZWaveValue closeValue = node->value(ZWaveValue::GenreUser, ZWaveValue::CommandClassSwitchMultilevel, 1, 2, ZWaveValue::TypeButton);
            closeValue.setValue(false);
            node->setValue(closeValue);
            info->finish(Thing::ThingErrorNoError);
            return;
        }
        if (info->action().actionTypeId() == rollerShadePercentageActionTypeId) {
            ZWaveValue percentageValue = node->value(ZWaveValue::GenreUser, ZWaveValue::CommandClassSwitchMultilevel, 1, 0, ZWaveValue::TypeByte);
            percentageValue.setValue(100 - info->action().param(rollerShadePercentageActionPercentageParamTypeId).value().toUInt());
            node->setValue(percentageValue);
            info->finish(Thing::ThingErrorNoError);
            return;
        }
    }

    info->finish(Thing::ThingErrorUnsupportedFeature);
}

void IntegrationPluginZWaveSpringsWindowFashions::thingRemoved(Thing *thing)
{
    ZWaveIntegrationPlugin::thingRemoved(thing);
}

