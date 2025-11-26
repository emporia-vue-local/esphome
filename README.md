For issues, please go to [the discussion board](https://github.com/emporia-vue-local/esphome/discussions).

**ESPHome Documentation:** https://esphome.io/

<details>
<summary>Instructions changelog</summary>

- 2025-11-24: update for main branch Vue 3 support
- 2023-11-01: suggest setting `restore: false`
- 2023-10-31: remove warning about flash, see https://github.com/emporia-vue-local/esphome/discussions/227#discussioncomment-7412125
- 2023-09-11: reduce logging verbosity
- 2023-09-03: revamp configuration for improved accuracy, thanks to [adam](https://www.technowizardry.net/2023/02/local-energy-monitoring-using-the-emporia-vue-2/) and [@kahrendt](https://github.com/kahrendt)
- 2023-06-11: fix buzzer with GND, move LED to HA config section, add template classes
- 2023-03-08: configuration example for net metering
- 2023-02-20: update style to modern home assistant, add buzzer support, add led support
- 2023-01-28: add frequency support
- 2023-01-18: increase flash write interval
- 2022-12-07: switch suggested branch back to dev
- 2022-07-30: add home assistant instructions & MQTT FAQ.
- 2022-07-16: mention using UART adaptor's RTS pin, thanks to @PanicRide
- 2022-07-02: mention mqtt is now supported
- 2022-04-30: bump software version number to 2022.4.0
- 2022-05-04: mention 64-bit ARM issues in FAQ

</details>

# Setting up Emporia Vue with ESPHome

This project was initially designed for the Vue 2 and now has full support for the Vue 3 enabled as well.
There are some significant differences in setup so please pay attention to your version in the instructions below.
(No support is known for the [gen 1](https://github.com/emporia-vue-local/esphome/discussions/335) devices.)

![example of hass setup](https://i.imgur.com/hC26j2M.png)

## What you need

- USB to serial converter module
  - I tested this with a cheap & generic CH340G adapter
- 4 male-to-female jumper wires
- 4 male pcb-mount headers
- Soldering iron & accessories
  - [some recommendations here](https://www.reddit.com/r/AskElectronics/wiki/soldering)
- [esptool.py](https://github.com/espressif/esptool) ([windows instructions](https://cyberblogspot.com/how-to-install-esptool-on-windows-10/), [generic instructions](https://docs.espressif.com/projects/esptool/en/latest/esp32/installation.html))
- Working ESPHome installation [(see "Getting started")](https://esphome.io/)

## Panel installation, part 1

You'll want to install the clamps & wiring harness into your panel following the instructions at https://www.emporiaenergy.com/installation-guides. At this time, place a label on each wire using masking tape & a pen rather than connecting them to the energy monitor.

Next, we need to figure out which circuits are on which phases, and in the case of multi-pole breakers, the multiplier. There should be a label like the following on your panel:
![panel phase diagram](https://i.imgur.com/GkoaLzp.jpeg)
For each clamp, you want to make a note of the following information:

- clamp number
- circuit number
- phase
- multiplier, if it is a multi-pole breaker

For the wiring harness, you'll want to make a note of which color cable matches which service main clamp (A, B, C).

## Backing up & flashing

**⚠️⚡ Do not power your Vue by mains when doing this flashing! It will not work & is deadly. Only connect your Vue to the mains with the enclosure closed. ⚡⚠️**

Pry the lever on one of the jumper cables up using a pencil or a needle or some other sharp thing. If your cables don't have a lever, cut one end of the cable & strip it using scissors or a knife.

![prying the lever on the jumper cable](https://i.imgur.com/BZJGdKq.jpg)![separated cable](https://i.imgur.com/eOc29M7.jpg)
You will then need to solder a serial header onto the programming port, so that it looks like this:

![closeup of the debug header pinout](https://i.imgur.com/NetVsQo.jpeg)
Plug the USB adapter in. Connect RX to RX, TX to TX, and GND to GND. Do not connect 5V or 3.3V at this time.

Plug in the unmodified end of the cable we modified above into the IO0 pin of the Emporia Vue 2.

Open a console window and test that `esptool.py version` works.

![photo of connected jumpers](https://i.imgur.com/TmB5PPV.jpeg)
Hold the modified end of the cable in IO0 to the metal shield on the ESP32. If you'd like, you can tape it down so that you have both hands free.

While holding it in place, connect 5V on your UART adapter to the `VCC_5V0` pin on the board.

If your TTL adapter has both the DTR and RTS pins exposed, you can let it automatically reboot the board and put the chip into flash mode when necessary. IO0 connects to DTR, and EN connects to RTS. In this case, you don't need to hold anything down.

### Doing a backup

With your other hand, run the following in the console: `esptool.py -b 921600 read_flash 0 0x800000 flash_contents.bin`. Successful completion of this step is _critical_ in case something goes wrong later. This file is necessary to restore the device to factory function.

If the above command fails, try again using `esptool.py -b 115200 read_flash 0 0x800000 flash_contents.bin`. If you're using an Apple Silicon (M1, M2, etc) CPU and it stops working after a certain percentage every time, try using a different machine

### Flashing new software

With your other hand, kick off the upload process. If you're using the command-line, `esphome run <yourfilename>.yaml`, otherwise click the button in the GUI. This will take a few minutes and install the new software on the Vue 2!

You'll see a bunch of errors like `Failed to read from sensor due to I2C error 3`, but that's fine, since they'll go away when it is installed into into the wall.

## Panel installation, part 2

Reassemble your Vue, and follow the instructions to plug everything in & started up!

## Getting a GUI

This project works best with Home Assistant. Follow these instructions to [connect the Vue 2 to Home Assistant](https://esphome.io/guides/getting_started_hassio.html#connecting-your-device-to-home-assistant).

Once you connect the Vue to Home Assistant, you can [configure the Home Assistant energy monitor functionallity](https://my.home-assistant.io/redirect/config_energy), as well as a variety of automations.


## ESPHome configuration

The two models (Vue 2 vs. Vue 3) vary slightly in their configuration needs.
Here are complete worked examples for each generation as a starting point.

See the next section for details about all the pieces,
including some notes on how to set up your own circuits.

### YAML Files

<details>
  <summary>Complete Vue 2 example</summary>

```
esphome:
  name: emporia-vue2
  friendly_name: Emporia Monitor (gen2 example)

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password

esp32:
  board: esp32dev
  framework:
    type: esp-idf
    version: recommended

external_components:
  - source: github://emporia-vue-local/esphome@dev
    components:
      - emporia_vue

api:
  encryption:
    key: !secret api_key
  services:
    - service: play_rtttl
      variables:
        song_str: string
      then:
        - rtttl.play:
            rtttl: !lambda 'return song_str;'

time:
  - platform: homeassistant

switch:
  - platform: restart
    name: Restart

ota:
  platform: esphome
  password: !secret ota_key

logger:
  logs:
    sensor: INFO

preferences:
  flash_write_interval: "48h"

rtttl:
  output: buzzer
  on_finished_playback:
    - logger.log: 'Song ended!'

button:
  - platform: template
    name: "Two Beeps"
    on_press:
      - rtttl.play: "two short:d=4,o=5,b=100:16e6,16e6"

light:
  - platform: status_led
    name: "D3_LED"
    pin: 23
    restore_mode: ALWAYS_ON
    entity_category: config

i2c:
  sda: 21
  scl: 22
  scan: false
  frequency: 400kHz
  timeout: 1ms
  id: i2c_a


substitutions:
  leg_1: "Phase A"
  leg_2: "Phase B"
  #leg_3 (`input: "C"` available but not used in this example)
  cir_1: "Circuit 1"
  cir_2: "Circuit 2"
  cir_3: "Circuit 3"
  cir_4: "Circuit 4"
  cir_5: "Circuit 5"
  cir_6: "Circuit 6"
  cir_7: "Circuit 7"
  cir_8: "Circuit 8"
  cir_9: "Circuit 9"
  cir_10: "Circuit 10"
  cir_11: "Circuit 11"
  cir_12: "Circuit 12"
  cir_13: "Circuit 13"
  cir_14: "Circuit 14"
  cir_15: "Circuit 15"
  cir_16: "Circuit 16"
  xtra: "Balance"  # displaying leftover/unmonitored energy (total minus circs 1–16)

.filters:
  # these are called references in YAML. They allow you to reuse
  # this configuration in each sensor, while only defining it once
  # you can adjust them here, split them up differently, even copy some inline…
  - &throttle_avg
    # average all raw readings together over a 5 second span before publishing
    throttle_average: 5s
  - &throttle_time
    # only send the most recent measurement every 60 seconds
    throttle: 60s
  - &neg
    # invert and filter out any values below 0.
    lambda: 'return max(-x, 0.0f);'
  - &pos
    # filter out any values below 0.
    lambda: 'return max(x, 0.0f);'
  - &abs
    # take the absolute value of the value
    lambda: 'return abs(x);'

sensor:
  - platform: emporia_vue
    variant: vue2
    i2c_id: i2c_a
    phases:
      - id: phase_a  # Verify that this specific phase/leg is connected to correct input wire color on device listed below
        input: BLACK # Vue device wire color
        calibration: 0.022  # just a starting point; may need adjusted to ensure accuracy
        # To calculate new calibration value use the formula <in-use calibration value> * <accurate voltage> / <reporting voltage>
        voltage:
          name: "${leg_1} Voltage"
          filters: [*throttle_avg, *pos]
        frequency:
          name: "${leg_1} Frequency"
          filters: [*throttle_avg, *pos]
      - id: phase_b  # see notes above
        input: RED
        calibration: 0.022
        voltage:
          name: "${leg_2} Voltage"
          filters: [*throttle_avg, *pos]
        phase_angle:
          name: "${leg_2} Phase Angle"
          filters: [*throttle_avg, *pos]

    ct_clamps:
      # These non-throttled power sensors are used for accurately calculating energy.
      # Recommend not to specify a `name` for any of the power sensors here — only the `id`!
      # This leaves them internal to ESPHome locally; post-processed data is sent to HA below.
      - phase_id: phase_a
        input: "A"  # Verify the CT going to these devices input also matches the phase/leg
        power:
          id: phase_a_power
          device_class: power
          filters: [*pos]
      - phase_id: phase_b
        input: "B"
        power:
          id: phase_b_power
          device_class: power
          filters: [*pos]
      # Pay close attention to set the `phase_id` for each breaker by matching it to the phase/leg it connects to in the panel
      - { phase_id: phase_a, input:  "1", power: { id:  cir1, filters: [ *neg, multiply: 2 ] } }
      - { phase_id: phase_a, input:  "2", power: { id:  cir2, filters: [ *neg, multiply: 2 ] } }
      - { phase_id: phase_a, input:  "3", power: { id:  cir3, filters: [ *neg ] } }
      - { phase_id: phase_b, input:  "4", power: { id:  cir4, filters: [ *neg ] } }
      - { phase_id: phase_a, input:  "5", power: { id:  cir5, filters: [ *neg ] } }
      - { phase_id: phase_b, input:  "6", power: { id:  cir6, filters: [ *neg ] } }
      - { phase_id: phase_a, input:  "7", power: { id:  cir7, filters: [ *neg ] } }
      - { phase_id: phase_b, input:  "8", power: { id:  cir8, filters: [ *neg ] } }
      - { phase_id: phase_a, input:  "9", power: { id:  cir9, filters: [ *neg ] } }
      - { phase_id: phase_b, input: "10", power: { id: cir10, filters: [ *neg ] } }
      - { phase_id: phase_a, input: "11", power: { id: cir11, filters: [ *neg ] } }
      - { phase_id: phase_b, input: "12", power: { id: cir12, filters: [ *neg ] } }
      - { phase_id: phase_a, input: "13", power: { id: cir13, filters: [ *neg ] } }
      - { phase_id: phase_b, input: "14", power: { id: cir14, filters: [ *neg ] } }
      - { phase_id: phase_a, input: "15", power: { id: cir15, filters: [ *neg ] } }
      - { phase_id: phase_b, input: "16", power: { id: cir16, filters: [ *neg ] } }
    on_update:
      then:
        - component.update: total_power
        - component.update: balance_power

  # these `copy` sensors filter and send the power state to HA
  - { platform: copy, name: "${leg_1} Power", source_id: phase_a_power, filters: *throttle_avg }
  - { platform: copy, name: "${leg_2} Power", source_id: phase_b_power, filters: *throttle_avg }
  - { platform: copy, name: "Total Power", source_id: total_power, filters: *throttle_avg }
  - { platform: copy, name: "${xtra} Power", source_id: balance_power, filters: *throttle_avg }
  - { platform: copy, name:  "${cir_1} Power", source_id:  cir1, filters: *throttle_avg }
  - { platform: copy, name:  "${cir_2} Power", source_id:  cir2, filters: *throttle_avg }
  - { platform: copy, name:  "${cir_3} Power", source_id:  cir3, filters: *throttle_avg }
  - { platform: copy, name:  "${cir_4} Power", source_id:  cir4, filters: *throttle_avg }
  - { platform: copy, name:  "${cir_5} Power", source_id:  cir5, filters: *throttle_avg }
  - { platform: copy, name:  "${cir_6} Power", source_id:  cir6, filters: *throttle_avg }
  - { platform: copy, name:  "${cir_7} Power", source_id:  cir7, filters: *throttle_avg }
  - { platform: copy, name:  "${cir_8} Power", source_id:  cir8, filters: *throttle_avg }
  - { platform: copy, name:  "${cir_9} Power", source_id:  cir9, filters: *throttle_avg }
  - { platform: copy, name: "${cir_10} Power", source_id: cir10, filters: *throttle_avg }
  - { platform: copy, name: "${cir_11} Power", source_id: cir11, filters: *throttle_avg }
  - { platform: copy, name: "${cir_12} Power", source_id: cir12, filters: *throttle_avg }
  - { platform: copy, name: "${cir_13} Power", source_id: cir13, filters: *throttle_avg }
  - { platform: copy, name: "${cir_14} Power", source_id: cir14, filters: *throttle_avg }
  - { platform: copy, name: "${cir_15} Power", source_id: cir15, filters: *throttle_avg }
  - { platform: copy, name: "${cir_16} Power", source_id: cir16, filters: *throttle_avg }

  - platform: template
    lambda: return id(phase_a_power).state + id(phase_b_power).state;
    update_interval: never   # will be updated after all power sensors update via on_update trigger
    id: total_power
    device_class: power
    state_class: measurement
    unit_of_measurement: "W"
  - platform: total_daily_energy
    name: "Total Daily Energy"
    power_id: total_power
    accuracy_decimals: 0
    restore: false
    filters: *throttle_time
  
  - platform: template
    lambda: !lambda |-
      return max(0.0f, id(total_power).state -
        id( cir1).state -
        id( cir2).state -
        id( cir3).state -
        id( cir4).state -
        id( cir5).state -
        id( cir6).state -
        id( cir7).state -
        id( cir8).state -
        id( cir9).state -
        id(cir10).state -
        id(cir11).state -
        id(cir12).state -
        id(cir13).state -
        id(cir14).state -
        id(cir15).state -
        id(cir16).state);
    update_interval: never   # still happens, but via `on_update` trigger (*after* all power sensors update)
    id: balance_power
    device_class: power
    state_class: measurement
    unit_of_measurement: "W"
  - platform: total_daily_energy
    name: "${xtra} Daily Energy"
    power_id: balance_power
    accuracy_decimals: 0
    restore: false
    filters: *throttle_time
  
  - { power_id:  cir1, platform: total_daily_energy, accuracy_decimals: 0, restore: false, name:  "${cir_1} Daily Energy", filters: *throttle_time }
  - { power_id:  cir2, platform: total_daily_energy, accuracy_decimals: 0, restore: false, name:  "${cir_2} Daily Energy", filters: *throttle_time }
  - { power_id:  cir3, platform: total_daily_energy, accuracy_decimals: 0, restore: false, name:  "${cir_3} Daily Energy", filters: *throttle_time }
  - { power_id:  cir4, platform: total_daily_energy, accuracy_decimals: 0, restore: false, name:  "${cir_4} Daily Energy", filters: *throttle_time }
  - { power_id:  cir5, platform: total_daily_energy, accuracy_decimals: 0, restore: false, name:  "${cir_5} Daily Energy", filters: *throttle_time }
  - { power_id:  cir6, platform: total_daily_energy, accuracy_decimals: 0, restore: false, name:  "${cir_6} Daily Energy", filters: *throttle_time }
  - { power_id:  cir7, platform: total_daily_energy, accuracy_decimals: 0, restore: false, name:  "${cir_7} Daily Energy", filters: *throttle_time }
  - { power_id:  cir8, platform: total_daily_energy, accuracy_decimals: 0, restore: false, name:  "${cir_8} Daily Energy", filters: *throttle_time }
  - { power_id:  cir9, platform: total_daily_energy, accuracy_decimals: 0, restore: false, name:  "${cir_9} Daily Energy", filters: *throttle_time }
  - { power_id: cir10, platform: total_daily_energy, accuracy_decimals: 0, restore: false, name: "${cir_10} Daily Energy", filters: *throttle_time }
  - { power_id: cir11, platform: total_daily_energy, accuracy_decimals: 0, restore: false, name: "${cir_11} Daily Energy", filters: *throttle_time }
  - { power_id: cir12, platform: total_daily_energy, accuracy_decimals: 0, restore: false, name: "${cir_12} Daily Energy", filters: *throttle_time }
  - { power_id: cir13, platform: total_daily_energy, accuracy_decimals: 0, restore: false, name: "${cir_13} Daily Energy", filters: *throttle_time }
  - { power_id: cir14, platform: total_daily_energy, accuracy_decimals: 0, restore: false, name: "${cir_14} Daily Energy", filters: *throttle_time }
  - { power_id: cir15, platform: total_daily_energy, accuracy_decimals: 0, restore: false, name: "${cir_15} Daily Energy", filters: *throttle_time }
  - { power_id: cir16, platform: total_daily_energy, accuracy_decimals: 0, restore: false, name: "${cir_16} Daily Energy", filters: *throttle_time }
```
</details>


<details>
  <summary>Complete Vue 3 example</summary>

```
esphome:
  name: emporia-vue3
  friendly_name: Emporia Monitor (gen3 example)

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password
  on_connect:
    - light.turn_on: wifi_led
  on_disconnect:
    - light.turn_off: wifi_led

esp32:
  board: esp32dev
  framework:
    type: esp-idf
    version: recommended

external_components:
  - source: github://emporia-vue-local/esphome@dev
    components:
      - emporia_vue

api:
  encryption:
    key: !secret api_key

time:
  - platform: homeassistant

switch:
  - platform: restart
    name: Restart

ota:
  platform: esphome
  password: !secret ota_key

logger:
  logs:
    sensor: INFO

preferences:
  flash_write_interval: "48h"

light:
  - platform: status_led
    id: wifi_led
    pin:
      number: 2
      ignore_strapping_warning: true
    restore_mode: RESTORE_DEFAULT_ON

  - platform: status_led
    id: ethernet_led
    pin: 4
    restore_mode: ALWAYS_OFF

i2c:
  sda:
    number: 5
    ignore_strapping_warning: true
  scl: 18
  scan: false
  frequency: 400kHz
  timeout: 1ms
  id: i2c_a


substitutions:
  leg_1: "Phase A"
  leg_2: "Phase B"
  cir_1: "Circuit 1"
  cir_2: "Circuit 2"
  cir_3: "Circuit 3"
  cir_4: "Circuit 4"
  cir_5: "Circuit 5"
  cir_6: "Circuit 6"
  cir_7: "Circuit 7"
  cir_8: "Circuit 8"
  cir_9: "Circuit 9"
  cir_10: "Circuit 10"
  cir_11: "Circuit 11"
  cir_12: "Circuit 12"
  cir_13: "Circuit 13"
  cir_14: "Circuit 14"
  cir_15: "Circuit 15"
  cir_16: "Circuit 16"
  xtra: "Balance"

.filters:
  - &throttle_avg
    # average all raw readings together over a 5 second span before publishing
    throttle_average: 5s
  - &throttle_time
    # only send the most recent measurement every 60 seconds
    throttle: 60s
  - &neg
    # invert and filter out any values below 0.
    lambda: 'return max(-x, 0.0f);'
  - &pos
    # filter out any values below 0.
    lambda: 'return max(x, 0.0f);'
  - &abs
    # take the absolute value of the value
    lambda: 'return abs(x);'

sensor:
  - platform: emporia_vue
    variant: vue3
    i2c_id: i2c_a
    phases:
      - id: phase_a  # Verify that this specific phase/leg is connected to correct input wire color on device listed below
        input: BLACK # Vue device wire color
        calibration: 0.01925 # serves as a reasonable starting point; may need adjusted to ensure accuracy
        # To calculate new calibration value use the formula <in-use calibration value> * <accurate voltage> / <reporting voltage>
        voltage:
          name: "${leg_1} Voltage"
          filters: [*throttle_avg, *pos]
        frequency:
          name: "${leg_1} Frequency"
          filters: [*throttle_avg, *pos]
      - id: phase_b  # see notes above
        input: RED
        calibration: 0.01925
        voltage:
          name: "${leg_2} Voltage"
          filters: [*throttle_avg, *pos]
        phase_angle:
          name: "${leg_2} Phase Angle"
          filters: [*throttle_avg, *pos]

    ct_clamps:
      # These non-throttled power sensors are used for accurately calculating energy.
      # Recommend not to specify a `name` for any of the power sensors here — only the `id`!
      # This leaves them internal to ESPHome locally; post-processed data is sent to HA below.
      - phase_id: phase_a
        input: "A"  # Verify the CT going to these devices input also matches the phase/leg
        power:
          id: phase_a_power
          device_class: power
          filters: [*pos]
      - phase_id: phase_b
        input: "B"
        power:
          id: phase_b_power
          device_class: power
          filters: [*pos]
      # Pay close attention to set the `phase_id` for each breaker by matching it to the phase/leg it connects to in the panel
      - { phase_id: phase_a, input:  "1", power: { id:  cir1, filters: [ *neg, multiply: 2 ] } }
      - { phase_id: phase_a, input:  "2", power: { id:  cir2, filters: [ *neg, multiply: 2 ] } }
      - { phase_id: phase_a, input:  "3", power: { id:  cir3, filters: [ *neg ] } }
      - { phase_id: phase_b, input:  "4", power: { id:  cir4, filters: [ *neg ] } }
      - { phase_id: phase_a, input:  "5", power: { id:  cir5, filters: [ *neg ] } }
      - { phase_id: phase_b, input:  "6", power: { id:  cir6, filters: [ *neg ] } }
      - { phase_id: phase_a, input:  "7", power: { id:  cir7, filters: [ *neg ] } }
      - { phase_id: phase_b, input:  "8", power: { id:  cir8, filters: [ *neg ] } }
      - { phase_id: phase_a, input:  "9", power: { id:  cir9, filters: [ *neg ] } }
      - { phase_id: phase_b, input: "10", power: { id: cir10, filters: [ *neg ] } }
      - { phase_id: phase_a, input: "11", power: { id: cir11, filters: [ *neg ] } }
      - { phase_id: phase_b, input: "12", power: { id: cir12, filters: [ *neg ] } }
      - { phase_id: phase_a, input: "13", power: { id: cir13, filters: [ *neg ] } }
      - { phase_id: phase_b, input: "14", power: { id: cir14, filters: [ *neg ] } }
      - { phase_id: phase_a, input: "15", power: { id: cir15, filters: [ *neg ] } }
      - { phase_id: phase_b, input: "16", power: { id: cir16, filters: [ *neg ] } }
    on_update:
      then:
        - component.update: total_power
        - component.update: balance_power

  # these `copy` sensors filter and send the power state to HA
  - { platform: copy, name: "${leg_1} Power", source_id: phase_a_power, filters: *throttle_avg }
  - { platform: copy, name: "${leg_2} Power", source_id: phase_b_power, filters: *throttle_avg }
  - { platform: copy, name: "Total Power", source_id: total_power, filters: *throttle_avg }
  - { platform: copy, name: "${xtra} Power", source_id: balance_power, filters: *throttle_avg }
  - { platform: copy, name:  "${cir_1} Power", source_id:  cir1, filters: *throttle_avg }
  - { platform: copy, name:  "${cir_2} Power", source_id:  cir2, filters: *throttle_avg }
  - { platform: copy, name:  "${cir_3} Power", source_id:  cir3, filters: *throttle_avg }
  - { platform: copy, name:  "${cir_4} Power", source_id:  cir4, filters: *throttle_avg }
  - { platform: copy, name:  "${cir_5} Power", source_id:  cir5, filters: *throttle_avg }
  - { platform: copy, name:  "${cir_6} Power", source_id:  cir6, filters: *throttle_avg }
  - { platform: copy, name:  "${cir_7} Power", source_id:  cir7, filters: *throttle_avg }
  - { platform: copy, name:  "${cir_8} Power", source_id:  cir8, filters: *throttle_avg }
  - { platform: copy, name:  "${cir_9} Power", source_id:  cir9, filters: *throttle_avg }
  - { platform: copy, name: "${cir_10} Power", source_id: cir10, filters: *throttle_avg }
  - { platform: copy, name: "${cir_11} Power", source_id: cir11, filters: *throttle_avg }
  - { platform: copy, name: "${cir_12} Power", source_id: cir12, filters: *throttle_avg }
  - { platform: copy, name: "${cir_13} Power", source_id: cir13, filters: *throttle_avg }
  - { platform: copy, name: "${cir_14} Power", source_id: cir14, filters: *throttle_avg }
  - { platform: copy, name: "${cir_15} Power", source_id: cir15, filters: *throttle_avg }
  - { platform: copy, name: "${cir_16} Power", source_id: cir16, filters: *throttle_avg }

  - platform: template
    lambda: return id(phase_a_power).state + id(phase_b_power).state;
    update_interval: never   # will be updated after all power sensors update via on_update trigger
    id: total_power
    device_class: power
    state_class: measurement
    unit_of_measurement: "W"
  - platform: total_daily_energy
    name: "Total Daily Energy"
    power_id: total_power
    accuracy_decimals: 0
    restore: false
    filters: *throttle_time
  
  - platform: template
    lambda: !lambda |-
      return max(0.0f, id(total_power).state -
        id( cir1).state -
        id( cir2).state -
        id( cir3).state -
        id( cir4).state -
        id( cir5).state -
        id( cir6).state -
        id( cir7).state -
        id( cir8).state -
        id( cir9).state -
        id(cir10).state -
        id(cir11).state -
        id(cir12).state -
        id(cir13).state -
        id(cir14).state -
        id(cir15).state -
        id(cir16).state);
    update_interval: never   # still happens, but via `on_update` trigger (*after* all power sensors update)
    id: balance_power
    device_class: power
    state_class: measurement
    unit_of_measurement: "W"
  - platform: total_daily_energy
    name: "${xtra} Daily Energy"
    power_id: balance_power
    accuracy_decimals: 0
    restore: false
    filters: *throttle_time
  
  - { power_id:  cir1, platform: total_daily_energy, accuracy_decimals: 0, restore: false, name:  "${cir_1} Daily Energy", filters: *throttle_time }
  - { power_id:  cir2, platform: total_daily_energy, accuracy_decimals: 0, restore: false, name:  "${cir_2} Daily Energy", filters: *throttle_time }
  - { power_id:  cir3, platform: total_daily_energy, accuracy_decimals: 0, restore: false, name:  "${cir_3} Daily Energy", filters: *throttle_time }
  - { power_id:  cir4, platform: total_daily_energy, accuracy_decimals: 0, restore: false, name:  "${cir_4} Daily Energy", filters: *throttle_time }
  - { power_id:  cir5, platform: total_daily_energy, accuracy_decimals: 0, restore: false, name:  "${cir_5} Daily Energy", filters: *throttle_time }
  - { power_id:  cir6, platform: total_daily_energy, accuracy_decimals: 0, restore: false, name:  "${cir_6} Daily Energy", filters: *throttle_time }
  - { power_id:  cir7, platform: total_daily_energy, accuracy_decimals: 0, restore: false, name:  "${cir_7} Daily Energy", filters: *throttle_time }
  - { power_id:  cir8, platform: total_daily_energy, accuracy_decimals: 0, restore: false, name:  "${cir_8} Daily Energy", filters: *throttle_time }
  - { power_id:  cir9, platform: total_daily_energy, accuracy_decimals: 0, restore: false, name:  "${cir_9} Daily Energy", filters: *throttle_time }
  - { power_id: cir10, platform: total_daily_energy, accuracy_decimals: 0, restore: false, name: "${cir_10} Daily Energy", filters: *throttle_time }
  - { power_id: cir11, platform: total_daily_energy, accuracy_decimals: 0, restore: false, name: "${cir_11} Daily Energy", filters: *throttle_time }
  - { power_id: cir12, platform: total_daily_energy, accuracy_decimals: 0, restore: false, name: "${cir_12} Daily Energy", filters: *throttle_time }
  - { power_id: cir13, platform: total_daily_energy, accuracy_decimals: 0, restore: false, name: "${cir_13} Daily Energy", filters: *throttle_time }
  - { power_id: cir14, platform: total_daily_energy, accuracy_decimals: 0, restore: false, name: "${cir_14} Daily Energy", filters: *throttle_time }
  - { power_id: cir15, platform: total_daily_energy, accuracy_decimals: 0, restore: false, name: "${cir_15} Daily Energy", filters: *throttle_time }
  - { power_id: cir16, platform: total_daily_energy, accuracy_decimals: 0, restore: false, name: "${cir_16} Daily Energy", filters: *throttle_time }
```
</details>

It's not too critical to get the entire configuration right on the first try, because you can usually update the board over Wi-Fi using [the ESPHome Dashboard](https://esphome.io/guides/getting_started_command_line.html#bonus-esphome-dashboard). You can even set up a [fallback Wi-Fi Access Point](https://esphome.io/components/wifi/#access-point-mode) if you're worried about getting your network settings right.

## Configuration details

The configuration process is very similar but depending on your model (Vue 2 vs. Vue 3) some sections will be different.

We've broken up these notes into basic things that are shared, sections that are different between the two generations, and the mostly-shared sensor customization.

### General setup (shared)

```
esphome:
  name: emporia-vue
  friendly_name: Emporia Monitor

esp32:
  board: esp32dev
  framework:
    type: esp-idf
    version: recommended

external_components:
  - source: github://emporia-vue-local/esphome@dev
    components:
      - emporia_vue

# Enable Home Assistant API…
api:
  encryption:
    key: !secret api_key

# …and use HA for setting our RTC time locally
time:
  - platform: homeassistant

# …and expose a (virtual) "switch" that HA can use to restart us
switch:
  - platform: restart
    name: Restart

# enable OTA updates after first flash
ota:
  platform: esphome
  password: !secret ota_key

# enable logging, with some customizations
logger:
  logs:
    # by default, every reading will be printed to the UART, which is very slow
    # This will disable printing the readings but keep other helpful messages
    sensor: INFO

preferences:
  # this might be overly slow, but do avoid wearing out the flash lifespan with too frequent of writes!
  # please also make sure `restore: false` is set on all `platform: total_daily_energy` sensors below.
  flash_write_interval: "48h"

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password
```

Most of this is fairly general ESPHome setup. We're using [secrets](https://esphome.io/guides/yaml/#secrets-and-the-secretsyaml-file) for all the WiFi and API/OTA configuration, but you can hard-code if you prefer. Likewise you can change the `name` and/or `friendly_name` to ± whatever you want within the [allowed syntax](https://esphome.io/components/esphome/#configuration-variables) for each.

There are a few specifics for this project we've added:

* adding in the external `emporia_vue` component to activate the code in this repo
* enabling the `time` component, needed for daily usage tracking (more below)
* some custom logging and flash settings

Do not use the `web_server` since it is not compatible with the `esp-idf` framework, and you will get odd error messages.


### Board setup (Vue 2)

The above common configuration needs some additional hardware items added in for the Vue 2 board:

```
api:
  # … snip …

  # (optional) add this to the shared API config above for buzzer features
  services:
    - service: play_rtttl
      variables:
        song_str: string
      then:
        - rtttl.play:
            rtttl: !lambda 'return song_str;'

rtttl:
  output: buzzer
  on_finished_playback:
    - logger.log: 'Song ended!'

button:
  - platform: template
    name: "Two Beeps"
    on_press:
      - rtttl.play: "two short:d=4,o=5,b=100:16e6,16e6"

light:
  - platform: status_led
    name: "D3_LED"
    pin: 23
    restore_mode: ALWAYS_ON
    entity_category: config

i2c:
  sda: 21
  scl: 22
  scan: false
  frequency: 400kHz
  timeout: 1ms
  id: i2c_a
```

This enables the piezo buzzer and lights on the board, as well as setting up the I2C bus for communication with the current monitoring sensor.


### Board setup (Vue 3)

For the Vue 3 board, the following hardware configuration is needed instead:

```
wifi:
  # … snip …

  # add these to the WiFi section:
  on_connect:
    - light.turn_on: wifi_led
  on_disconnect:
    - light.turn_off: wifi_led

light:
  - platform: status_led
    id: wifi_led
    pin:
      number: 2
      ignore_strapping_warning: true
    restore_mode: RESTORE_DEFAULT_ON

  - platform: status_led
    id: ethernet_led
    pin: 4
    restore_mode: ALWAYS_OFF

i2c:
  sda:
    number: 5
    ignore_strapping_warning: true
  scl: 18
  scan: false
  frequency: 400kHz
  timeout: 1ms
  id: i2c_a
```

The main differences are that the I2C and LED lights use different pins on this board. And there is no built-in buzzer like the gen 2 had. There is also an Ethernet device available.

**⚠️⚡ We do not recommend, and many jurisdictions may prohibit, running low voltage wiring (network cables) in/out of an electrical panel. Consult a local authority or qualified electrician! ⚡⚠️**

WiFi is strongly recommended (and much better tested/supported) but the Ethernet hardware may also be supported by ESPHome at your own risk:

```
ethernet:
  type: RTL8201
  mdc_pin: GPIO32
  mdio_pin: GPIO33
  clk_mode: GPIO0_IN
  on_connect:
    - light.turn_on: ethernet_led
  on_disconnect:
    - light.turn_off: ethernet_led
```

### Sensor setup (mostly shared)

This is **only a starting point**. You'll need to tailor this to your own system using the information you've collected in Panel installation, part 1.

We've broken out some [substitutions](https://esphome.io/components/substitutions/) for the repeated circuit names but note that you will still need to go to each circuit within the `ct_clamps` section and make sure for each:

* the `phase_id` is set to the correct leg of your panel for that circuit
  * see [this explanation](https://github.com/emporia-vue-local/esphome/discussions/332#discussioncomment-12257818) 
* you have the right either `*pos` or `*neg` filter depending on which direction the CT reads
  * note that the suggested filters also [truncate out noise](https://github.com/emporia-vue-local/esphome/discussions/354) that would lead to negative energy readings. for solar or other generation this may not be what you want!
* if you want to adjust the reading, e.g. it is common to `multiply: 2` if you are monitoring half of a double-pole breaker
  * compare [this alternative](https://github.com/emporia-vue-local/esphome/discussions/55#discussioncomment-5018829) wiring

Make sure you use the correct `variant: vue2` or `variant: vue3` to your platform!

```
substitutions:
  leg_1: "Phase A"
  leg_2: "Phase B"
  #leg_3 (`input: "C"` available but not used in this example)
  cir_1: "Circuit 1"
  cir_2: "Circuit 2"
  cir_3: "Circuit 3"
  cir_4: "Circuit 4"
  cir_5: "Circuit 5"
  cir_6: "Circuit 6"
  cir_7: "Circuit 7"
  cir_8: "Circuit 8"
  cir_9: "Circuit 9"
  cir_10: "Circuit 10"
  cir_11: "Circuit 11"
  cir_12: "Circuit 12"
  cir_13: "Circuit 13"
  cir_14: "Circuit 14"
  cir_15: "Circuit 15"
  cir_16: "Circuit 16"
  xtra: "Balance"  # displaying leftover/unmonitored energy (total minus circs 1–16)

.filters:
  # these are called references in YAML. They allow you to reuse
  # this configuration in each sensor, while only defining it once
  # you can adjust them here, split them up differently, even copy some inline…
  - &throttle_avg
    # average all raw readings together over a 5 second span before publishing
    throttle_average: 5s
  - &throttle_time
    # only send the most recent measurement every 60 seconds
    throttle: 60s
  - &neg
    # invert and filter out any values below 0.
    lambda: 'return max(-x, 0.0f);'
  - &pos
    # filter out any values below 0.
    lambda: 'return max(x, 0.0f);'
  - &abs
    # take the absolute value of the value
    lambda: 'return abs(x);'

sensor:
  - platform: emporia_vue
    variant: FIXME # `vue2` or `vue3`
    i2c_id: i2c_a
    phases:
      - id: phase_a  # Verify that this specific phase/leg is connected to correct input wire color on device listed below
        input: BLACK # Vue device wire color
        calibration: 0.01925 # 0.022 is used as the default as vue2 starting point; either may need adjusted to ensure accuracy
        # To calculate new calibration value use the formula <in-use calibration value> * <accurate voltage> / <reporting voltage>
        voltage:
          name: "${leg_1} Voltage"
          filters: [*throttle_avg, *pos]
        frequency:
          name: "${leg_1} Frequency"
          filters: [*throttle_avg, *pos]
      - id: phase_b  # see notes above
        input: RED
        calibration: 0.01925
        voltage:
          name: "${leg_2} Voltage"
          filters: [*throttle_avg, *pos]
        phase_angle:
          name: "${leg_2} Phase Angle"
          filters: [*throttle_avg, *pos]

    ct_clamps:
      # These non-throttled power sensors are used for accurately calculating energy.
      # Recommend not to specify a `name` for any of the power sensors here — only the `id`!
      # This leaves them internal to ESPHome locally; post-processed data is sent to HA below.
      - phase_id: phase_a
        input: "A"  # Verify the CT going to these devices input also matches the phase/leg
        power:
          id: phase_a_power
          device_class: power
          filters: [*pos]
      - phase_id: phase_b
        input: "B"
        power:
          id: phase_b_power
          device_class: power
          filters: [*pos]
      # Pay close attention to set the `phase_id` for each breaker by matching it to the phase/leg it connects to in the panel
      - { phase_id: phase_a, input:  "1", power: { id:  cir1, filters: [ *neg, multiply: 2 ] } }
      - { phase_id: phase_a, input:  "2", power: { id:  cir2, filters: [ *neg, multiply: 2 ] } }
      - { phase_id: phase_a, input:  "3", power: { id:  cir3, filters: [ *neg ] } }
      - { phase_id: phase_b, input:  "4", power: { id:  cir4, filters: [ *neg ] } }
      - { phase_id: phase_a, input:  "5", power: { id:  cir5, filters: [ *neg ] } }
      - { phase_id: phase_b, input:  "6", power: { id:  cir6, filters: [ *neg ] } }
      - { phase_id: phase_a, input:  "7", power: { id:  cir7, filters: [ *neg ] } }
      - { phase_id: phase_b, input:  "8", power: { id:  cir8, filters: [ *neg ] } }
      - { phase_id: phase_a, input:  "9", power: { id:  cir9, filters: [ *neg ] } }
      - { phase_id: phase_b, input: "10", power: { id: cir10, filters: [ *neg ] } }
      - { phase_id: phase_a, input: "11", power: { id: cir11, filters: [ *neg ] } }
      - { phase_id: phase_b, input: "12", power: { id: cir12, filters: [ *neg ] } }
      - { phase_id: phase_a, input: "13", power: { id: cir13, filters: [ *neg ] } }
      - { phase_id: phase_b, input: "14", power: { id: cir14, filters: [ *neg ] } }
      - { phase_id: phase_a, input: "15", power: { id: cir15, filters: [ *neg ] } }
      - { phase_id: phase_b, input: "16", power: { id: cir16, filters: [ *neg ] } }
    on_update:
      then:
        - component.update: total_power
        - component.update: balance_power

  # these `copy` sensors filter and send the power state to HA
  - { platform: copy, name: "${leg_1} Power", source_id: phase_a_power, filters: *throttle_avg }
  - { platform: copy, name: "${leg_2} Power", source_id: phase_b_power, filters: *throttle_avg }
  - { platform: copy, name: "Total Power", source_id: total_power, filters: *throttle_avg }
  - { platform: copy, name: "${xtra} Power", source_id: balance_power, filters: *throttle_avg }
  - { platform: copy, name:  "${cir_1} Power", source_id:  cir1, filters: *throttle_avg }
  - { platform: copy, name:  "${cir_2} Power", source_id:  cir2, filters: *throttle_avg }
  - { platform: copy, name:  "${cir_3} Power", source_id:  cir3, filters: *throttle_avg }
  - { platform: copy, name:  "${cir_4} Power", source_id:  cir4, filters: *throttle_avg }
  - { platform: copy, name:  "${cir_5} Power", source_id:  cir5, filters: *throttle_avg }
  - { platform: copy, name:  "${cir_6} Power", source_id:  cir6, filters: *throttle_avg }
  - { platform: copy, name:  "${cir_7} Power", source_id:  cir7, filters: *throttle_avg }
  - { platform: copy, name:  "${cir_8} Power", source_id:  cir8, filters: *throttle_avg }
  - { platform: copy, name:  "${cir_9} Power", source_id:  cir9, filters: *throttle_avg }
  - { platform: copy, name: "${cir_10} Power", source_id: cir10, filters: *throttle_avg }
  - { platform: copy, name: "${cir_11} Power", source_id: cir11, filters: *throttle_avg }
  - { platform: copy, name: "${cir_12} Power", source_id: cir12, filters: *throttle_avg }
  - { platform: copy, name: "${cir_13} Power", source_id: cir13, filters: *throttle_avg }
  - { platform: copy, name: "${cir_14} Power", source_id: cir14, filters: *throttle_avg }
  - { platform: copy, name: "${cir_15} Power", source_id: cir15, filters: *throttle_avg }
  - { platform: copy, name: "${cir_16} Power", source_id: cir16, filters: *throttle_avg }

  - platform: template
    lambda: return id(phase_a_power).state + id(phase_b_power).state;
    update_interval: never   # will be updated after all power sensors update via on_update trigger
    id: total_power
    device_class: power
    state_class: measurement
    unit_of_measurement: "W"
  - platform: total_daily_energy
    name: "Total Daily Energy"
    power_id: total_power
    accuracy_decimals: 0
    restore: false
    filters: *throttle_time
  
  - platform: template
    lambda: !lambda |-
      return max(0.0f, id(total_power).state -
        id( cir1).state -
        id( cir2).state -
        id( cir3).state -
        id( cir4).state -
        id( cir5).state -
        id( cir6).state -
        id( cir7).state -
        id( cir8).state -
        id( cir9).state -
        id(cir10).state -
        id(cir11).state -
        id(cir12).state -
        id(cir13).state -
        id(cir14).state -
        id(cir15).state -
        id(cir16).state);
    update_interval: never   # still happens, but via `on_update` trigger (*after* all power sensors update)
    id: balance_power
    device_class: power
    state_class: measurement
    unit_of_measurement: "W"
  - platform: total_daily_energy
    name: "${xtra} Daily Energy"
    power_id: balance_power
    accuracy_decimals: 0
    restore: false
    filters: *throttle_time
  
  - { power_id:  cir1, platform: total_daily_energy, accuracy_decimals: 0, restore: false, name:  "${cir_1} Daily Energy", filters: *throttle_time }
  - { power_id:  cir2, platform: total_daily_energy, accuracy_decimals: 0, restore: false, name:  "${cir_2} Daily Energy", filters: *throttle_time }
  - { power_id:  cir3, platform: total_daily_energy, accuracy_decimals: 0, restore: false, name:  "${cir_3} Daily Energy", filters: *throttle_time }
  - { power_id:  cir4, platform: total_daily_energy, accuracy_decimals: 0, restore: false, name:  "${cir_4} Daily Energy", filters: *throttle_time }
  - { power_id:  cir5, platform: total_daily_energy, accuracy_decimals: 0, restore: false, name:  "${cir_5} Daily Energy", filters: *throttle_time }
  - { power_id:  cir6, platform: total_daily_energy, accuracy_decimals: 0, restore: false, name:  "${cir_6} Daily Energy", filters: *throttle_time }
  - { power_id:  cir7, platform: total_daily_energy, accuracy_decimals: 0, restore: false, name:  "${cir_7} Daily Energy", filters: *throttle_time }
  - { power_id:  cir8, platform: total_daily_energy, accuracy_decimals: 0, restore: false, name:  "${cir_8} Daily Energy", filters: *throttle_time }
  - { power_id:  cir9, platform: total_daily_energy, accuracy_decimals: 0, restore: false, name:  "${cir_9} Daily Energy", filters: *throttle_time }
  - { power_id: cir10, platform: total_daily_energy, accuracy_decimals: 0, restore: false, name: "${cir_10} Daily Energy", filters: *throttle_time }
  - { power_id: cir11, platform: total_daily_energy, accuracy_decimals: 0, restore: false, name: "${cir_11} Daily Energy", filters: *throttle_time }
  - { power_id: cir12, platform: total_daily_energy, accuracy_decimals: 0, restore: false, name: "${cir_12} Daily Energy", filters: *throttle_time }
  - { power_id: cir13, platform: total_daily_energy, accuracy_decimals: 0, restore: false, name: "${cir_13} Daily Energy", filters: *throttle_time }
  - { power_id: cir14, platform: total_daily_energy, accuracy_decimals: 0, restore: false, name: "${cir_14} Daily Energy", filters: *throttle_time }
  - { power_id: cir15, platform: total_daily_energy, accuracy_decimals: 0, restore: false, name: "${cir_15} Daily Energy", filters: *throttle_time }
  - { power_id: cir16, platform: total_daily_energy, accuracy_decimals: 0, restore: false, name: "${cir_16} Daily Energy", filters: *throttle_time }
```

There's a lot here, don't get overwhelmed! A lot of it is simply repeated 16x for each of the circuits. The general outline is:

* per-circuit labels ([split out](https://esphome.io/guides/yaml/#substitutions) for convenience)
* some shared filters (more on these later)
* the core `emporia_vue` sensor configuration:
  * starting with the per-phase A/B(/C) *voltage* monitoring [sometimes called L1/L2/L3 poles]
  * followed by the per-phase A/B(/C) *current* sensing CTs  [again intended for the mains/poles into the panel]
  * and then the 16x individual circuit CT sensors (really the same, only indented differently, as the per-phase ones)
  * NOTE: all these subsensors update [every 240ms](https://github.com/emporia-vue-local/esphome/discussions/333)!
* templates to prepare the readings for more efficient Home Assistant data collection:
  * each of the current sensors live power reading copied into HA via a `throttle_avg` (5sec) filter
  * each of the sensors integrated into a total daily energy reading via a 1m `throttle_time` (1min) filter
  * also a `balance_power` template (which you can remove if not wanted) which subtracts the 16x individual from the A+B total [for comparison](https://github.com/emporia-vue-local/esphome/discussions/329)

Note especially the `throttle_avg` we set up. This is optional, but since we get a reading every 240ms, it is helpful to average these readings together so that we don't need to store such dense, noisy, data in Home Assistant. Similarly note the "Total Power", "Total Daily Energy", and "Circuit x Daily Energy". These are needed for the Home Assistant energy system, which requires daily kWh numbers. These are (again optionally) processed through a customizable `throttle_time` filter so HA gets a reading every minute.

Okay, so that's still a lot. The general flow is:

```
(Emporia-local)     (Copied to Home Assistant)
  raw readings  -->  smoothed power (5s)
    (240ms)     \->  daily integral (60s)

total = A + B
balance = total - (1 + 2 + 3 + … + 16)
```

## Net metering addendum (shared, in case of solar or other generation)

To configure energy returned to the grid for net metering ([more info here](https://www.nrel.gov/state-local-tribal/basics-net-metering.html)), you need to add the following configuration:

```yaml
sensor:
  - platform: emporia_vue
    ct_clamps:
      - phase_id: phase_a
        input: "A"  # Verify the CT going to this device input also matches the phase/leg
        power:
          name: "Phase A Power Return"
          id: phase_a_power_return
          filters: [*throttle_avg, *invert]  # This measures energy uploaded to grid on phase A
      - phase_id: phase_b
        input: "B"  # Verify the CT going to this device input also matches the phase/leg
        power:
          name: "Phase B Power Return"
          id: phase_b_power_return
          filters: [*throttle_avg, *invert]  # This measures energy uploaded to grid on phase B
  - platform: template
    name: "Total Power Return"
    lambda: return id(phase_a_power_return).state + id(phase_b_power_return).state;
    update_interval: 1s
    id: total_power_return
    device_class: power
    state_class: measurement
    unit_of_measurement: "W"
  - platform: total_daily_energy
    name: "Total Daily Energy Return"
    power_id: total_power_return
    accuracy_decimals: 0

```

Your solar sensors' configuration depends on your setup (single phase, split phase, 3-phase). The following example shows a split-phase installation using ct clamps 15 and 16:

```yaml
sensor:
  - platform: template
    name: "Solar Power"
    lambda: return id(cir15).state + id(cir16).state;
    id: solar_power
    device_class: power
    state_class: measurement
    unit_of_measurement: "W"
  - platform: total_daily_energy
    name: "Solar Daily Energy"
    power_id: solar_power
    accuracy_decimals: 0
```

## FAQ

### What is MQTT?

MQTT is an alternative way of communicating the readings. If you need it, you already know, and it is not required for use with Home Assistant.

### How do I use this with MQTT?

There's now support for MQTT with this integration thanks to the hard work of the ESPHome folks! Please reference [MQTT Client Component](https://esphome.io/components/mqtt.html) for how to get this set up.

### I'm getting negative values

- You may have put that clamp on the wire backwards
- You may have selected the wrong phase in the configuration

### I've recorded negative energy values and I want to reset them

Sensor values are saved to the esp32 flash. You can reset all sensors by implementing a [factory reset button](https://esphome.io/components/button/factory_reset.html).

### The readings on one or two of my sensors are crazy

Sometimes the CTs aren't fully plugged into the 3.5mm jacks on the Vue. It's often not an issue with the initial install, but with stuff getting jostled around as you put things back together.

This issue will often manifest as jumps between 0W and some other wattage for no reason.

Open up the panel, and make sure every connector is fully inserted into the Vue. Check if the problem is solved before putting the panel cover back on.

### My data readings go up and down

If your readings are within ±1W, then they're within the expected margin of error. The filters are designed to smooth out noise like this, and it's expected as no physical system can be perfect.

If the readings are significant outside of that, there may be a problem.

### I'm seeing zeros on certain current clamps

First off, you will want to remove all filters for that sensor. Replace `filters: [ *throttle_avg, *pos ]`, etc, with `filters: []`.

If your data is hovering around 0, then you either don't have any load on that circuit or there's some other issue that hasn't come up before.

If you're seeing negative data, it could be a few things:

- First off, make sure you've properly installed the clamps according to the instructions. The L side of the clamp should point towards the load. For solar systems or similar, keep in mind that current flows from the solar panel to your electrical panel, not the other way.
- Make sure you've selected the correct phase in the configuration. You will get negative _and_ nonsense power readings if you select the wrong phase. You can't negate the data through a filter and expect it to be correct.

When you're done troubleshooting, remember to place the filters back.

### I'm using a 64-bit Pi & can't compile!

Some users have successfully managed to build this on a 64-bit Pi: https://github.com/emporia-vue-local/esphome/discussions/147

~If you're using a 64-bit ARM OS, unfortunately you are unable to build this. It's not a limitation with this project, but a limitation with the upstream PlatformIO toolchains.~

You'll see an error like

```
Could not find the package with 'platformio/toolchain-esp32ulp @ ~1.22851.0' requirements for your system 'linux_aarch64'
```

You can try using a different computer. 32-bit and 64-bit x86 computers are both compatible (most laptops & desktops).
