For issues, please go to [the discussion board](https://github.com/emporia-vue-local/esphome/discussions).

⚠️NOTICE⚠️: If you have already flashed your device with a previous version of the config, I'd strongly encourage you to add `flash_write_interval: "48h"` from below to your config to preserve the flash memory's health and your ability to update the device in the future.

**ESPHome Documentation:** https://esphome.io/

<details>
<summary>Instructions changelog</summary>

- 2023-01-28: add frequency support
- 2023-01-18: increase flash write interval
- 2022-12-07: switch suggested branch back to dev
- 2022-07-30: add home assistant instructions & MQTT FAQ.
- 2022-07-16: mention using UART adaptor's RTS pin, thanks to @PanicRide 
- 2022-07-02: mention mqtt is now supported
- 2022-04-30: bump software version number to 2022.4.0
- 2022-05-04: mention 64-bit ARM issues in FAQ
	
</details>

# Setting up Emporia Vue 2 with ESPHome
![example of hass setup](https://i.imgur.com/hC26j2M.png)

## What you need

- USB to serial converter module
	- I tested this with a cheap & generic CH340G adapter
- 4 male-to-female jumper wires
- 4 male pcb-mount headers
- Soldering iron & accessories
	- [some recommendations here](https://www.reddit.com/r/AskElectronics/wiki/soldering) 
- [esptool.py](https://github.com/espressif/esptool) ([windows instructions](https://cyberblogspot.com/how-to-install-esptool-on-windows-10/), [generic instructions](https://docs.espressif.com/projects/esptool/en/latest/esp32/installation.html))
- ESPHome image

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

## Writing configuration

Here's a starting point for a configuration:
```
esphome:
  name: emporiavue2

external_components:
  - source: github://emporia-vue-local/esphome@dev
    components: [ emporia_vue ]

esp32:
  board: esp32dev
  framework:
    type: esp-idf
    version: recommended

# Enable Home Assistant API
api: {"password": "<ota password>"}
ota: {"password": "<ota password>"}

# Enable logging
logger:

wifi:
  ssid: "<wifi ssid>"
  password: "<wifi password>"

preferences:
  # the default of 1min is far too short--flash chip is rated
  # for approx 100k writes.
  flash_write_interval: "48h"
i2c:
  sda: 21
  scl: 22
  scan: false
  frequency: 200kHz  # recommended range is 50-200kHz
  id: i2c_a
time:
  - platform: sntp
    id: my_time
    # Adjust to your local timezone.  Impacts on when day is wrapped for daily energy calculations
    timezone: 'America/Los_Angeles'
    
# these are called references in YAML. They allow you to reuse
# this configuration in each sensor, while only defining it once
.defaultfilters:
  - &moving_avg
    # we capture a new sample every 0.24 seconds, so the time can
    # be calculated from the number of samples as n * 0.24.
    sliding_window_moving_average:
      # we average over the past 2.88 seconds
      window_size: 12
      # we push a new value every 1.44 seconds
      send_every: 6
  - &invert
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
    i2c_id: i2c_a
    phases:
      - id: phase_a  # Verify that this specific phase/leg is connected to correct input wire color on device listed below
        input: BLACK  # Vue device wire color
        calibration: 0.022  # 0.022 is used as the default as starting point but may need adjusted to ensure accuracy
        # To calculate new calibration value use the formula <in-use calibration value> * <accurate voltage> / <reporting voltage>
        voltage:
          name: "Phase A Voltage"
          filters: [*moving_avg, *pos]
        frequency:
          name: "Phase A Frequency"
          filters: [*moving_avg, *pos]
      - id: phase_b  # Verify that this specific phase/leg is connected to correct input wire color on device listed below
        input: RED  # Vue device wire color
        calibration: 0.022  # 0.022 is used as the default as starting point but may need adjusted to ensure accuracy
        # To calculate new calibration value use the formula <in-use calibration value> * <accurate voltage> / <reporting voltage>
        voltage:
          name: "Phase B Voltage"
          filters: [*moving_avg, *pos]
        phase_angle:
          name: "Phase B Phase Angle"
          filters: [*moving_avg, *pos]
    ct_clamps:
      - phase_id: phase_a
        input: "A"  # Verify the CT going to this device input also matches the phase/leg
        power:
          name: "Phase A Power"
          id: phase_a_power
          device_class: power
          filters: [*moving_avg, *pos]
      - phase_id: phase_b
        input: "B"  # Verify the CT going to this device input also matches the phase/leg
        power:
          name: "Phase B Power"
          id: phase_b_power
          device_class: power
          filters: [*moving_avg, *pos]
      # Pay close attention to set the phase_id for each breaker by matching it to the phase/leg it connects to in the panel
      - { phase_id: phase_a, input:  "1", power: { name:  "Circuit 1 Power", id:  cir1, filters: [ *moving_avg, *pos ] } }
      - { phase_id: phase_b, input:  "2", power: { name:  "Circuit 2 Power", id:  cir2, filters: [ *moving_avg, *pos ] } }
      - { phase_id: phase_a, input:  "3", power: { name:  "Circuit 3 Power", id:  cir3, filters: [ *moving_avg, *pos ] } }
      - { phase_id: phase_a, input:  "4", power: { name:  "Circuit 4 Power", id:  cir4, filters: [ *moving_avg, *pos ] } }
      - { phase_id: phase_a, input:  "5", power: { name:  "Circuit 5 Power", id:  cir5, filters: [ *moving_avg, *pos, multiply: 2 ] } }
      - { phase_id: phase_a, input:  "6", power: { name:  "Circuit 6 Power", id:  cir6, filters: [ *moving_avg, *pos, multiply: 2 ] } }
      - { phase_id: phase_a, input:  "7", power: { name:  "Circuit 7 Power", id:  cir7, filters: [ *moving_avg, *pos, multiply: 2 ] } }
      - { phase_id: phase_b, input:  "8", power: { name:  "Circuit 8 Power", id:  cir8, filters: [ *moving_avg, *pos ] } }
      - { phase_id: phase_b, input:  "9", power: { name:  "Circuit 9 Power", id:  cir9, filters: [ *moving_avg, *pos ] } }
      - { phase_id: phase_b, input: "10", power: { name: "Circuit 10 Power", id: cir10, filters: [ *moving_avg, *pos ] } }
      - { phase_id: phase_a, input: "11", power: { name: "Circuit 11 Power", id: cir11, filters: [ *moving_avg, *pos, multiply: 2 ] } }
      - { phase_id: phase_a, input: "12", power: { name: "Circuit 12 Power", id: cir12, filters: [ *moving_avg, *pos, multiply: 2 ] } }
      - { phase_id: phase_a, input: "13", power: { name: "Circuit 13 Power", id: cir13, filters: [ *moving_avg, *pos ] } }
      - { phase_id: phase_a, input: "14", power: { name: "Circuit 14 Power", id: cir14, filters: [ *moving_avg, *pos ] } }
      - { phase_id: phase_b, input: "15", power: { name: "Circuit 15 Power", id: cir15, filters: [ *moving_avg, *pos ] } }
      - { phase_id: phase_a, input: "16", power: { name: "Circuit 16 Power", id: cir16, filters: [ *moving_avg, *pos ] } }
  - platform: template
    name: "Total Power"
    lambda: return id(phase_a_power).state + id(phase_b_power).state;
    update_interval: 1s
    id: total_power
    unit_of_measurement: "W"
  - platform: total_daily_energy
    name: "Total Daily Energy"
    power_id: total_power
    accuracy_decimals: 0
  - { power_id:  cir1, platform: total_daily_energy, accuracy_decimals: 0, name:  "Circuit 1 Daily Energy" }
  - { power_id:  cir2, platform: total_daily_energy, accuracy_decimals: 0, name:  "Circuit 2 Daily Energy" }
  - { power_id:  cir3, platform: total_daily_energy, accuracy_decimals: 0, name:  "Circuit 3 Daily Energy" }
  - { power_id:  cir4, platform: total_daily_energy, accuracy_decimals: 0, name:  "Circuit 4 Daily Energy" }
  - { power_id:  cir5, platform: total_daily_energy, accuracy_decimals: 0, name:  "Circuit 5 Daily Energy" }
  - { power_id:  cir6, platform: total_daily_energy, accuracy_decimals: 0, name:  "Circuit 6 Daily Energy" }
  - { power_id:  cir7, platform: total_daily_energy, accuracy_decimals: 0, name:  "Circuit 7 Daily Energy" }
  - { power_id:  cir8, platform: total_daily_energy, accuracy_decimals: 0, name:  "Circuit 8 Daily Energy" }
  - { power_id:  cir9, platform: total_daily_energy, accuracy_decimals: 0, name:  "Circuit 9 Daily Energy" }
  - { power_id: cir10, platform: total_daily_energy, accuracy_decimals: 0, name: "Circuit 10 Daily Energy" }
  - { power_id: cir11, platform: total_daily_energy, accuracy_decimals: 0, name: "Circuit 11 Daily Energy" }
  - { power_id: cir12, platform: total_daily_energy, accuracy_decimals: 0, name: "Circuit 12 Daily Energy" }
  - { power_id: cir13, platform: total_daily_energy, accuracy_decimals: 0, name: "Circuit 13 Daily Energy" }
  - { power_id: cir14, platform: total_daily_energy, accuracy_decimals: 0, name: "Circuit 14 Daily Energy" }
  - { power_id: cir15, platform: total_daily_energy, accuracy_decimals: 0, name: "Circuit 15 Daily Energy" }
  - { power_id: cir16, platform: total_daily_energy, accuracy_decimals: 0, name: "Circuit 16 Daily Energy" }
```

You'll want to replace `<ota password>`, `<wifi ssid>`, and `<wifi password>` with a unique password, and your wifi credentials, respectively.

You'll also want to update the `sensor` section of the configuration using the information you've collected in *Panel installation, part 1*.

Note the `sliding_window_moving_average`. This is optional, but since we get a reading every 240ms, it is helpful to average these readings together so that we don't need to store such dense, noisy, data in Home Assistant.

Note the "Total Power", "Total Daily Energy", and "Circuit x Daily Energy". This is needed for the Home Assistant energy system, which requires daily kWh numbers.

Do not use the `web_server` since it is not compatible with the `esp-idf` framework, and you will get odd error messages.

It's not too critical to get this right on the first try, because you can update the board over WiFi using [the ESPHome Dashboard](https://esphome.io/guides/getting_started_command_line.html#bonus-esphome-dashboard).

## Backing up & flashing the Vue 2

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

With your other hand, run the following in the console: `esptool.py -b 921600 read_flash 0 0x800000 flash_contents.bin`. Successful completion of this step is *critical* in case something goes wrong later. This file is necessary to restore the device to factory function.

If the above command fails, try again using `esptool.py -b 115200 read_flash 0 0x800000 flash_contents.bin`.

### Flashing new software

With your other hand, run the following in the console: `esphome run vue2.yaml`. This will take a few minutes, and install the new software on the Vue 2!

You'll see a bunch of errors like `Failed to read from sensor due to I2C error 3`, but that's fine, since they'll go away when it is installed into into the wall.

## Panel installation, part 2

Reassemble to Vue 2, and follow the instructions to plug everything in & started up!

## Getting a GUI

This project works best with Home Assistant. Follow these instructions to [connect the Vue 2 to Home Assistant](https://esphome.io/guides/getting_started_hassio.html#connecting-your-device-to-home-assistant).

Once you connect the Vue to Home Assistant, you can [configure the Home Assistant energy monitor functionallity](https://my.home-assistant.io/redirect/config_energy), as well as a variety of automations.

## FAQ

### What is MQTT?

MQTT is an alternative way of communicating the readings. If you need it, you already know, and it is not required for use with Home Assistant.

### How do I use this with MQTT?

There's now support for MQTT with this integration thanks to the hard work of the ESPHome folks! Please reference [MQTT Client Component](https://esphome.io/components/mqtt.html) for how to get this set up.

### I'm getting negative values

- You may have put that clamp on the wire backwards
- You may have selected the wrong phase in the configuration

### The readings on one or two of my sensors are crazy

Sometimes the CTs aren't fully plugged into the 3.5mm jacks on the Vue. It's often not an issue with the initial install, but with stuff getting jostled around as you put things back together.

This issue will often manifest as jumps between 0W and some other wattage for no reason.

Open up the panel, and make sure every connector is fully inserted into the Vue. Check if the problem is solved before putting the panel cover back on.

### My data readings go up and down

If your readings are within ±1W, then they're within the expected margin of error. The filters are designed to smooth out noise like this, and it's expected as no physical system can be perfect.

If the readings are significant outside of that, there may be a problem.

### I'm seeing zeros on certain current clamps

First off, you will want to remove all filters for that sensor. Replace `filters: [ *moving_avg, *pos ]`, etc, with `filters: []`.

If your data is hovering around 0, then you either don't have any load on that circuit or there's some other issue that hasn't come up before.

If you're seeing negative data, it could be a few things:

- First off, make sure you've properly installed the clamps according to the instructions. The L side of the clamp should point towards the load. For solar systems or similar, keep in mind that current flows from the solar panel to your electrical panel, not the other way.
- Make sure you've selected the correct phase in the configuration. You will get negative *and* nonsense power readings if you select the wrong phase. You can't negate the data through a filter and expect it to be correct.

When you're done troubleshooting, remember to place the filters back.

### I'm using a 64-bit Pi & can't compile!

If you're using a 64-bit ARM OS, unfortunately you are unable to build this. It's not a limitation with this project, but a limitation with the upstream PlatformIO toolchains.

You'll see an error like

```
Could not find the package with 'platformio/toolchain-esp32ulp @ ~1.22851.0' requirements for your system 'linux_aarch64'
```

You can try using a different computer. 32-bit and 64-bit x86 computers are both compatible (most laptops & desktops). 
