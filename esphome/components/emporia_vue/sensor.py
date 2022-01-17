from esphome.components import sensor, i2c
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import (
    CONF_CALIBRATION,
    CONF_CURRENT,
    CONF_ID,
    CONF_INPUT,
    CONF_POWER,
    CONF_VOLTAGE,
    DEVICE_CLASS_CURRENT,
    DEVICE_CLASS_POWER,
    DEVICE_CLASS_VOLTAGE,
    STATE_CLASS_MEASUREMENT,
    UNIT_AMPERE,
    UNIT_WATT,
    UNIT_VOLT,
)

CONF_CT_CLAMPS = "ct_clamps"
CONF_PHASES = "phases"
CONF_PHASE_ID = "phase_id"

CODEOWNERS = ["@flaviut", "@Maelstrom96", "@krconv"]
ESP_PLATFORMS = ["esp-idf"]
DEPENDENCIES = ["i2c"]
AUTOLOAD = ["sensor"]

emporia_vue_ns = cg.esphome_ns.namespace("emporia_vue")
EmporiaVueComponent = emporia_vue_ns.class_(
    "EmporiaVueComponent", cg.Component, i2c.I2CDevice
)
PhaseConfig = emporia_vue_ns.class_("PhaseConfig")
CTClampConfig = emporia_vue_ns.class_("CTClampConfig")

PhaseInputWire = emporia_vue_ns.enum("PhaseInputWire")
PHASE_INPUT = {
    "BLACK": PhaseInputWire.BLACK,
    "RED": PhaseInputWire.RED,
    "BLUE": PhaseInputWire.BLUE,
}

CTInputPort = emporia_vue_ns.enum("CTInputPort")
CT_INPUT = {
    "A": CTInputPort.A,
    "B": CTInputPort.B,
    "C": CTInputPort.C,
    "1": CTInputPort.ONE,
    "2": CTInputPort.TWO,
    "3": CTInputPort.THREE,
    "4": CTInputPort.FOUR,
    "5": CTInputPort.FIVE,
    "6": CTInputPort.SIX,
    "7": CTInputPort.SEVEN,
    "8": CTInputPort.EIGHT,
    "9": CTInputPort.NINE,
    "10": CTInputPort.TEN,
    "11": CTInputPort.ELEVEN,
    "12": CTInputPort.TWELVE,
    "13": CTInputPort.THIRTEEN,
    "14": CTInputPort.FOURTEEN,
    "15": CTInputPort.FIFTEEN,
    "16": CTInputPort.SIXTEEN,
}

SCHEMA_CT_CLAMP = {
    cv.GenerateID(): cv.declare_id(CTClampConfig),
    cv.Required(CONF_PHASE_ID): cv.use_id(PhaseConfig),
    cv.Required(CONF_INPUT): cv.enum(CT_INPUT),
    cv.Optional(CONF_POWER): sensor.sensor_schema(
        unit_of_measurement=UNIT_WATT,
        device_class=DEVICE_CLASS_POWER,
        state_class=STATE_CLASS_MEASUREMENT,
        accuracy_decimals=1,
    ),
    cv.Optional(CONF_CURRENT): sensor.sensor_schema(
        unit_of_measurement=UNIT_AMPERE,
        device_class=DEVICE_CLASS_CURRENT,
        state_class=STATE_CLASS_MEASUREMENT,
        accuracy_decimals=2,
    ),
}

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(EmporiaVueComponent),
            cv.Required(CONF_PHASES): cv.ensure_list(
                {
                    cv.Required(CONF_ID): cv.declare_id(PhaseConfig),
                    cv.Required(CONF_INPUT): cv.enum(PHASE_INPUT),
                    cv.Optional(CONF_CALIBRATION, default=0.022): cv.zero_to_one_float,
                    cv.Optional(CONF_VOLTAGE): sensor.sensor_schema(
                        unit_of_measurement=UNIT_VOLT,
                        device_class=DEVICE_CLASS_VOLTAGE,
                        state_class=STATE_CLASS_MEASUREMENT,
                        accuracy_decimals=1,
                    ),
                }
            ),
            cv.Required(CONF_CT_CLAMPS): cv.ensure_list(SCHEMA_CT_CLAMP),
        },
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(i2c.i2c_device_schema(0x64)),
    cv.only_with_esp_idf,
    cv.only_on_esp32,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    phases = []
    for phase_config in config[CONF_PHASES]:
        phase_var = cg.new_Pvariable(phase_config[CONF_ID], PhaseConfig())
        cg.add(phase_var.set_input_wire(phase_config[CONF_INPUT]))
        cg.add(phase_var.set_calibration(phase_config[CONF_CALIBRATION]))

        if CONF_VOLTAGE in phase_config:
            voltage_sensor = await sensor.new_sensor(phase_config[CONF_VOLTAGE])
            cg.add(phase_var.set_voltage_sensor(voltage_sensor))

        phases.append(phase_var)
    cg.add(var.set_phases(phases))

    ct_clamps = []
    for ct_config in config[CONF_CT_CLAMPS]:
        ct_clamp_var = cg.new_Pvariable(ct_config[CONF_ID], CTClampConfig())
        phase_var = await cg.get_variable(ct_config[CONF_PHASE_ID])
        cg.add(ct_clamp_var.set_phase(phase_var))
        cg.add(ct_clamp_var.set_input_port(ct_config[CONF_INPUT]))

        if CONF_POWER in ct_config:
            power_sensor = await sensor.new_sensor(ct_config[CONF_POWER])
            cg.add(ct_clamp_var.set_power_sensor(power_sensor))

        if CONF_CURRENT in ct_config:
            current_sensor = await sensor.new_sensor(ct_config[CONF_CURRENT])
            cg.add(ct_clamp_var.set_current_sensor(current_sensor))

        ct_clamps.append(ct_clamp_var)
    cg.add(var.set_ct_clamps(ct_clamps))
