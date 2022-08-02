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

#include "integrationpluginzwavefibaro.h"
#include "plugininfo.h"

#include <hardware/zwave/zwavehardwareresource.h>

#include <QDebug>

IntegrationPluginZWaveFibaro::IntegrationPluginZWaveFibaro(): ZWaveIntegrationPlugin(ZWaveHardwareResource::HandlerTypeVendor, dcZWaveFibaro())
{
}

QString IntegrationPluginZWaveFibaro::name() const
{
    return "Fibaro";
}

bool IntegrationPluginZWaveFibaro::handleNode(ZWaveNode *node)
{
    qCDebug(dcZWaveFibaro()) << "Handle node for Fibaro:" << node;

    if (node->manufacturerId() != 0x010f) {
        return false;
    }

    // FGWPE/F Wall Plug (0x1000)
    if (node->productId() == 0x1000) {
        createThing(powerSocketThingClassId, node);
        return true;
    }

    // 0x1002 = EU version, 0x2002 = US version
    if (node->productId() == 0x1002 || node->productId() == 0x2002) {
        createThing(motionSensorThingClassId, node);
        return true;
    }

    return false;
}

void IntegrationPluginZWaveFibaro::setupThing(ThingSetupInfo *info)
{
    Thing *thing = info->thing();

    if (!manageNode(thing)) {
        qCWarning(dcZWaveFibaro()) << "Failed to claim node during setup.";
        info->finish(Thing::ThingErrorHardwareNotAvailable);
        return;
    }

    ZWaveNode *node = nodeForThing(thing);

    if (thing->thingClassId() == powerSocketThingClassId) {

        thing->setSettingValue(powerSocketSettingsAlwaysOnModeParamTypeId, node->value(ZWaveValue::GenreConfig, ZWaveValue::CommandClassConfiguration, 1, 1, ZWaveValue::TypeList).valueListSelection() == 0);
        thing->setSettingValue(powerSocketSettingsRestoreModeParamTypeId, node->value(ZWaveValue::GenreConfig, ZWaveValue::CommandClassConfiguration, 1, 16, ZWaveValue::TypeList).valueListSelection() == 1);
        QVariantList ledModes = thing->thingClass().settingsTypes().findById(powerSocketSettingsLedModeParamTypeId).allowedValues();
        thing->setSettingValue(powerSocketSettingsLedModeParamTypeId, ledModes.at(node->value(ZWaveValue::GenreConfig, ZWaveValue::CommandClassConfiguration, 1, 61, ZWaveValue::TypeList).valueListSelection()));

        info->finish(Thing::ThingErrorNoError);

        thing->setStateValue(powerSocketPowerStateTypeId, node->value(ZWaveValue::GenreUser, ZWaveValue::CommandClassSwitchBinary, 1, 0, ZWaveValue::TypeBool).value().toBool());
        thing->setStateValue(powerSocketCurrentPowerStateTypeId, node->value(ZWaveValue::GenreUser, ZWaveValue::CommandClassSensorMultilevel, 1, 4, ZWaveValue::TypeDecimal).value().toDouble());
        thing->setStateValue(powerSocketTotalEnergyConsumedStateTypeId, node->value(ZWaveValue::GenreUser, ZWaveValue::CommandClassMeter, 1, 0, ZWaveValue::TypeDecimal).value().toDouble());

        connect(node, &ZWaveNode::valueChanged, thing, [thing, ledModes](const ZWaveValue &value){
            if (value.genre() == ZWaveValue::GenreConfig && value.commandClass() == ZWaveValue::CommandClassConfiguration && value.index() == 1) {
                thing->setSettingValue(powerSocketSettingsAlwaysOnModeParamTypeId, value.valueListSelection() == 0);
            }
            if (value.genre() == ZWaveValue::GenreConfig && value.commandClass() == ZWaveValue::CommandClassConfiguration && value.index() == 16) {
                thing->setSettingValue(powerSocketSettingsRestoreModeParamTypeId, value.valueListSelection() == 1);
            }
            if (value.genre() == ZWaveValue::GenreConfig && value.commandClass() == ZWaveValue::CommandClassConfiguration && value.index() == 61) {
                thing->setSettingValue(powerSocketSettingsLedModeParamTypeId, ledModes.at(value.valueListSelection()));
            }

            if (value.genre() == ZWaveValue::GenreUser && value.commandClass() == ZWaveValue::CommandClassSwitchBinary) {
                thing->setStateValue(powerSocketPowerStateTypeId, value.value().toBool());
            }
            if (value.genre() == ZWaveValue::GenreUser && value.commandClass() == ZWaveValue::CommandClassSensorMultilevel) {
                thing->setStateValue(powerSocketCurrentPowerStateTypeId, value.value().toDouble());
            } if (value.genre() == ZWaveValue::GenreUser && value.commandClass() == ZWaveValue::CommandClassMeter && value.index() == 0) {
                thing->setStateValue(powerSocketTotalEnergyConsumedStateTypeId, value.value().toDouble());
            }
        });

        connect(info->thing(), &Thing::settingChanged, node, [node, ledModes](const ParamTypeId &settingId, const QVariant &value){
            if (settingId == powerSocketSettingsAlwaysOnModeParamTypeId) {
                ZWaveValue alwaysOnValue = node->value(ZWaveValue::GenreConfig, ZWaveValue::CommandClassConfiguration, 1, 1, ZWaveValue::TypeList);
                alwaysOnValue.selectListValue(value.toBool() ? 0 : 1);
                node->setValue(alwaysOnValue);
            }
            if (settingId == powerSocketSettingsRestoreModeParamTypeId) {
                ZWaveValue restoreModeValue = node->value(ZWaveValue::GenreConfig, ZWaveValue::CommandClassConfiguration, 1, 16, ZWaveValue::TypeList);
                restoreModeValue.selectListValue(value.toBool() ? 1 : 0);
                node->setValue(restoreModeValue);
            }
            if (settingId == powerSocketSettingsLedModeParamTypeId) {
                ZWaveValue ledModeValue = node->value(ZWaveValue::GenreConfig, ZWaveValue::CommandClassConfiguration, 1, 61, ZWaveValue::TypeList);
                ledModeValue.selectListValue(ledModes.indexOf(value.toString()));
                node->setValue(ledModeValue);
            }
        });

        return;
    }

    if (thing->thingClassId() == motionSensorThingClassId) {
        qCDebug(dcZWaveFibaro()) << "Handling Fibaro motion sensor" << node;

        uint value = node->value(ZWaveValue::GenreConfig, ZWaveValue::CommandClassConfiguration, 1, 1, ZWaveValue::TypeByte).value().toUInt();
        double ratio = 1.0 - ((1.0 * value - 8) / (255 - 8));
        qCDebug(dcZWaveFibaro()) << "**************" << value << ratio << ratio * 100;
        thing->setSettingValue(motionSensorSettingsSensitivityParamTypeId, ratio * 100);

        thing->setStateValue(motionSensorBatteryLevelStateTypeId, node->value(ZWaveValue::GenreUser, ZWaveValue::CommandClassBattery, 1, 0, ZWaveValue::TypeByte).value().toInt());
        thing->setStateValue(motionSensorBatteryCriticalStateTypeId, node->value(ZWaveValue::GenreUser, ZWaveValue::CommandClassBattery, 1, 0, ZWaveValue::TypeByte).value().toInt() < 5);
        thing->setStateValue(motionSensorIsPresentStateTypeId, node->value(ZWaveValue::GenreUser, ZWaveValue::CommandClassAlarm, 1, 0, ZWaveValue::TypeInt).value().toInt() == 0);
        thing->setStateValue(motionSensorTamperedStateTypeId, node->value(ZWaveValue::GenreUser, ZWaveValue::CommandClassAlarm, 1, 7, ZWaveValue::TypeList).valueListSelection() == 1);
        thing->setStateValue(motionSensorTemperatureStateTypeId, node->value(ZWaveValue::GenreUser, ZWaveValue::CommandClassSensorMultilevel, 1, 1, ZWaveValue::TypeDecimal).value().toDouble());
        thing->setStateValue(motionSensorLightIntensityStateTypeId, node->value(ZWaveValue::GenreUser, ZWaveValue::CommandClassSensorMultilevel, 1, 3, ZWaveValue::TypeDecimal).value().toDouble());
        thing->setStateValue(motionSensorSeismicIntensityStateTypeId, node->value(ZWaveValue::GenreUser, ZWaveValue::CommandClassSensorMultilevel, 1, 25, ZWaveValue::TypeDecimal).value().toDouble());

        info->finish(Thing::ThingErrorNoError);

        connect(node, &ZWaveNode::valueChanged, thing, [thing](const ZWaveValue &value){
            qCDebug(dcZWaveFibaro()) << "Value changed:" << value;

            if (value.genre() == ZWaveValue::GenreUser && value.commandClass() == ZWaveValue::CommandClassBattery) {
                thing->setStateValue(motionSensorBatteryLevelStateTypeId, value.value().toInt());
                thing->setStateValue(motionSensorBatteryCriticalStateTypeId, value.value().toInt() < 5);
            }
            if (value.genre() == ZWaveValue::GenreUser && value.commandClass() == ZWaveValue::CommandClassAlarm && value.index() == 0) {
                thing->setStateValue(motionSensorIsPresentStateTypeId, value.value().toInt() == 0);
            }
            if (value.genre() == ZWaveValue::GenreUser && value.commandClass() == ZWaveValue::CommandClassAlarm && value.index() == 7) {
                thing->setStateValue(motionSensorTamperedStateTypeId, value.valueListSelection() == 1);
            }
            if (value.genre() == ZWaveValue::GenreUser && value.commandClass() == ZWaveValue::CommandClassSensorMultilevel && value.index() == 1) {
                thing->setStateValue(motionSensorTemperatureStateTypeId, value.value().toDouble());
            }
            if (value.genre() == ZWaveValue::GenreUser && value.commandClass() == ZWaveValue::CommandClassSensorMultilevel && value.index() == 3) {
                thing->setStateValue(motionSensorLightIntensityStateTypeId, value.value().toDouble());
            }
            if (value.genre() == ZWaveValue::GenreUser && value.commandClass() == ZWaveValue::CommandClassSensorMultilevel && value.index() == 25) {
                thing->setStateValue(motionSensorSeismicIntensityStateTypeId, value.value().toDouble());
            }
        });

        connect(thing, &Thing::settingChanged, node, [node](const ParamTypeId &settingId, const QVariant &value){
            if (settingId == motionSensorSettingsSensitivityParamTypeId) {
                ZWaveValue intensityValue = node->value(ZWaveValue::GenreConfig, ZWaveValue::CommandClassConfiguration, 1, 1, ZWaveValue::TypeByte);
                double ratio = (value.toDouble() / 100 * (255 - 8));
                uint val = (255 - 8) - ratio + 8;
                qCDebug(dcZWaveFibaro()) << "**************" << value << ratio << val;

                intensityValue.setValue(val);
                node->setValue(intensityValue);
            }
        });

        return;
    }


    info->finish(Thing::ThingErrorNoError);
}

void IntegrationPluginZWaveFibaro::executeAction(ThingActionInfo *info)
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

    info->finish(Thing::ThingErrorUnsupportedFeature);
}

void IntegrationPluginZWaveFibaro::thingRemoved(Thing *thing)
{
    ZWaveIntegrationPlugin::thingRemoved(thing);
}

