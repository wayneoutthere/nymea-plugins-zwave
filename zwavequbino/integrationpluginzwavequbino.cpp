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

#include "integrationpluginzwavequbino.h"
#include "plugininfo.h"

#include <hardware/zwave/zwavehardwareresource.h>

#include <QDebug>

IntegrationPluginZWaveQubino::IntegrationPluginZWaveQubino(): ZWaveIntegrationPlugin(ZWaveHardwareResource::HandlerTypeVendor, dcZWaveQubino())
{
}

QString IntegrationPluginZWaveQubino::name() const
{
    return "Qubino";
}

bool IntegrationPluginZWaveQubino::handleNode(ZWaveNode *node)
{
    qCDebug(dcZWaveQubino()) << "Handle node for Qubino" << node;

    if (node->manufacturerId() != 0x0159) {
        return false;
    }

    // ZNMHCD Flush shutter
    if (node->productId() == 0x0052) {
        createThing(flushShutterThingClassId, node);
        return true;
    }

    return false;
}

void IntegrationPluginZWaveQubino::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    if (!manageNode(thing)) {
        qCWarning(dcZWaveQubino()) << "Failed to claim node during setup.";
        info->finish(Thing::ThingErrorHardwareNotAvailable);
        return;
    }

    ZWaveNode *node = nodeForThing(thing);

    if (thing->thingClassId() == flushShutterThingClassId) {

        info->finish(Thing::ThingErrorNoError);

        thing->setStateValue(flushShutterPercentageStateTypeId, node->value(ZWaveValue::GenreUser, ZWaveValue::CommandClassSwitchMultilevel, 1, 0, ZWaveValue::TypeByte).value().toUInt() * 100 / 255);

        connect(node, &ZWaveNode::valueChanged, thing, [thing](const ZWaveValue &value){
            if (value.genre() == ZWaveValue::GenreConfig && value.commandClass() == ZWaveValue::CommandClassConfiguration && value.index() == 74) {
                qCDebug(dcZWaveQubino()) << "Open/Close time value changed:" << value.value();
                thing->setSettingValue(flushShutterSettingsOpenCloseTimeParamTypeId, value.value());
            }

            else if (value.genre() == ZWaveValue::GenreUser && value.commandClass() == ZWaveValue::CommandClassSwitchMultilevel && value.index() == 0) {
                qCDebug(dcZWaveQubino()) << "Level value changed:" << value.value();
                thing->setStateValue(flushShutterPercentageStateTypeId, 100 - value.value().toUInt());
            }

            else {
                qCWarning(dcZWaveQubino()) << "Unhandled value changed" << value;
            }
        });

        connect(info->thing(), &Thing::settingChanged, node, [node](const ParamTypeId &settingId, const QVariant &value){
            qCDebug(dcZWaveQubino()) << "Setting qubino settings!!!";
            if (settingId == flushShutterSettingsOpenCloseTimeParamTypeId) {
                ZWaveValue openCloseDurationValue = node->value(ZWaveValue::GenreConfig, ZWaveValue::CommandClassConfiguration, 1, 74, ZWaveValue::TypeShort);
                openCloseDurationValue.setValue(value);
                node->setValue(openCloseDurationValue);
            }
        });

        return;
    }

    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginZWaveQubino::executeAction(ThingActionInfo *info)
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

    if (thing->thingClassId() == flushShutterThingClassId) {
        if (info->action().actionTypeId() == flushShutterOpenActionTypeId) {
            ZWaveValue powerValue = node->value(ZWaveValue::GenreUser, ZWaveValue::CommandClassSwitchMultilevel, 1, 1, ZWaveValue::TypeButton);
            powerValue.setValue(true);
            node->setValue(powerValue);
            info->finish(Thing::ThingErrorNoError);
            return;
        }
        if (info->action().actionTypeId() == flushShutterCloseActionTypeId) {
            ZWaveValue powerValue = node->value(ZWaveValue::GenreUser, ZWaveValue::CommandClassSwitchMultilevel, 1, 2, ZWaveValue::TypeButton);
            powerValue.setValue(true);
            node->setValue(powerValue);
            info->finish(Thing::ThingErrorNoError);
            return;
        }
        if (info->action().actionTypeId() == flushShutterStopActionTypeId) {
            ZWaveValue openValue = node->value(ZWaveValue::GenreUser, ZWaveValue::CommandClassSwitchMultilevel, 1, 1, ZWaveValue::TypeButton);
            openValue.setValue(false);
            node->setValue(openValue);

            ZWaveValue closeValue = node->value(ZWaveValue::GenreUser, ZWaveValue::CommandClassSwitchMultilevel, 1, 2, ZWaveValue::TypeButton);
            closeValue.setValue(false);
            node->setValue(closeValue);
            info->finish(Thing::ThingErrorNoError);
            return;
        }

        if (info->action().actionTypeId() == flushShutterPercentageActionTypeId) {
            ZWaveValue percentageValue = node->value(ZWaveValue::GenreUser, ZWaveValue::CommandClassSwitchMultilevel, 1, 0, ZWaveValue::TypeByte);
            percentageValue.setValue(100 - info->action().paramValue(flushShutterPercentageActionPercentageParamTypeId).toUInt());
            node->setValue(percentageValue);

            info->finish(Thing::ThingErrorNoError);
            return;
        }

        if (info->action().actionTypeId() == flushShutterCalibrationActionTypeId) {
            ZWaveValue calibrationValue = node->value(ZWaveValue::GenreConfig, ZWaveValue::CommandClassConfiguration, 1, 78, ZWaveValue::TypeByte);
            calibrationValue.setValue(info->action().paramValue(flushShutterCalibrationActionCalibrationParamTypeId).toBool());
            node->setValue(calibrationValue);
            info->finish(Thing::ThingErrorNoError);
            return;
        }
    }


    info->finish(Thing::ThingErrorUnsupportedFeature);
}

void IntegrationPluginZWaveQubino::thingRemoved(Thing *thing)
{
    ZWaveIntegrationPlugin::thingRemoved(thing);
}

