# The super awesome thermostat

## Introduction

Let's face it - all these thermostat products you can buy today are any or many of:

1. stupid expensive,
1. flawed in some way
1. don't do exactly what you want (see above)
1. aren't hackable
1. sell your data to some huge company

Let's address that, shall we?

![HACK THE PLANET!!!](./hack-the-planet.gif)

## Getting started

Clone this as follows:

```shell
git clone --recursive https://github.com/mattcaron/thermostat.git
```

This will get this repo and all the submodules uses (such as the Espressif repositories).

## Components

### Thermometer puck sensors

This is intended to be pretty turnkey. Build it, flash it, configure it,
stick it on the wall. It will send MQTT messages to your broker (see below)
which you then consume in NodeRED (also below) and make it do what you want.

**Status:** In progress. See the `sensors` subdirectory.

**Roadmap:**

1. Hardware selection: Completed
1. Firmware: Done (v1.0.0)
   1. Console: Done
   1. Config: Done
   1. OneWire to DS18B20: Done
   1. WiFi refactor: Done
   1. MQTT: Done
   1. Power saving deep sleep: Done
1. Wiring diagram: Not started
1. Case design: Completed

**TODO:**

(May never be implemented)

1. Implement CLI command completion hinting (see iperf example code).

**Notes:**

1. **MAKE SURE YOU DO NOT CONNECT TO BOTH USB AND BATTERIES**. Seriously - the
   batteries are going to push somewhere between 3V and 4.5V and the USB is
   going to push 5V so it's likely to try to charge the batteries and that is
   probably not going to end well. Remove the batteries, *then* plug in USB.
   This should be a pretty easy mistake to avoid since this assumes a battery
   holder with screw holes behind the batteries - in order to upgrade firmware,
   you need to take the whole thing apart, and the first step to doing that is
   pulling out the batteries.
1. When you flash the firmware:
   1. The config is retained, as it is stored in a different flash partition
      than the firmware.
   1. You need to pull out the jumper wire between D0 and RST. This connection
      is necessary for deep sleep, but messes up the automatic reset circuit
      for flashing.
   1. If the delay between plugging in USB and starting the flashing process is
      too long, the board will go to deep sleep (from which it will not actually
      wake - see above) and not flash properly. If the flashing fails, hit the
      reset button, then start the flash process again.
1. I originally implemented this using 9 bit DS18B20 resolution, but the 0.5 C
   resolution was lacking. After profiling where we were spending our time, the
   overwhelming majority of it was spent waiting to bring ip WiFi, get an IP,
   etc. Therefore, it cost us very little in terms of power to kick off a full
   resolution (12 bit) conversion, giving us 0.0625C resolution.

**Known bugs:**

(May never be implemented)

1. If you save the config at the wrong time, it can try to take down WiFi when
   it is already being brought down so the unit can go to sleep. This causes an
   ESP_ERROR_CHECK to fail (because you can't bring down something that's
   already being brought down), causing a panic reboot. Eventually, I'll get
   around to modifying it so as to check for that specific error condition and
   ignore it. Note that, when this happens, the config is saved before it
   reboots, so it's not like you lose anything. Further, this is not likely to
   occur outside of initial setup - hence why I haven't bothered to
   fix it.

#### BOM

Example links are provided for clarity, in cases where the hardware is uncommon.

##### Hardware list

You will need one of each of these items for each "puck".

1. NodeMCU Wireless Mini D1 ESP-12F Board
   * Either v2 or v3 should work fine.
   * https://www.amazon.com/dp/B073CQVFLK
1. 3 AAA battery holder with leads
   * https://www.amazon.com/gp/product/B0156V1JGQ
1. DS18B20 - Get the actual Maxim one.
   * I used the genuine article here, because I tried a couple of knockoffs and
    they lacked the ability to save configuration and were less precise than the
    actual Maxim product.
      * The above is accurate, but if you run it at 12 bit resolution (the
        default), it doesn't matter, and a generic knockoff will probably work
        fine. Your mileage may vary. See `sensors/main/temperature.c` and set
        `SENSOR_CONFIG_REG_VALUE` and `MEASUREMENT_DELAY_MS` accordingly.
   * https://www.mouser.com/ProductDetail/?qs=7H2Jq%252ByxpJKpIDCbiq4lfg%3D%3D
1. Wire of appropriate gauge - one spool each of red and black (for power and
   ground) and another for signal (orange, yellow, blue, etc.) in 22 AWD should
   work fine here.
1. Heat shrink tubing for the above - you're going to be soldering to the ends
   of the DS18B20 sensor and you'll want to heat shrink over them to ensure you
   don't get any shorts.
   * Alternatively, if you want to do slightly less soldering, you could find a
     3 pin pigtail lead with breadboard (2.54mm) pin spacing, plug the DS18B20's
     into the plug end and solder the other end to the ESP board. But, wire is
     cheaper and I rather enjoy soldering - it lets me catch up on my Oak
     Island. Your call.

#### Enclosure

The enclosure is meant to be 3D printed. See the `enclosures` subdirectory. The
files are all generated from a single SCAD file. If you want to re-render or
modify them, feel free to do so. OpenSCAD is what I use.

However, this is not strictly necessary, because I've already exported the 3
pieces as `.stl`s. They are meant to print flat, without supports, in the
orientation in which they are presented, on an FDM printer. They should print
fine in any common 3D printing material. PLA will probably work fine, given the
limited environmental effects to which they will be exposed. I used PETG, my
rationale being that it should survive the temperature fluctuations and UV
exposure a little better than PLA, but mainly because I wanted an excuse to get
PETG printing dialed in on my printer - it's still pretty stringy, but vastly
improved versus the beginning of the project.

In addition to whatever filament you will be using, you will also need
sufficient self-tapping screws to fasten the battery holder to the lid of the
enclosure - with the screws passing through the divider. I used M3 x 12mm wood
screws, as that was the head profile for which my battery holders were made.

Please note that the holes in both the lid and divider are merely intended to be
pilot holes. Since I do not know what size screw you plan to use, I cannot
ensure the correct dimension. As such, 2 drill bits will be needed.

The first must be larger than your screw and, ideally, the same diameter as the
holes in the battery holder. The goal here is to have the screw threads *not*
engage with the plastic in either the divider or the battery holder.

The second should be the diameter of your screws sans threads - the idea being
that the self-tapping screws will cut walls into the printed standoff parts of
the lid.

Given that you will be removing some material from the walls of the standoffs so
the screws do not crack the plastic, it is necessary to ensure thick enough
walls for them to not be to thin. I recommend a minimum of 2mm thick walls when
printing the lid.

The divider is intended to ensure that the board and thermostat stay in
position, with the CPU located under the upper vent and the DS18B20 positioned
just above the lower vent. The idea here is that the CPU will heat up and the
warm air will vent out the top. Ambient air will be drawn in and over the
temperature sensor, theoretically giving a more accurate reading (and surely
more accurate than if the DS18B20 was on above the CPU, with the CPU sitting
there baking it). The horizontal protrusions into which one screws the battery
case and divider are intended to serve as a heat brake as well.

Finally, note that there are no clips on this assembly, as it is meant to be
stuck to the wall with some variety of double stick tape (I like the 3M command
strips). As such, the base is stuck to the wall and the lid, batteries, board,
etc. just pull straight off. They are held in place by gravity and a small
amount of friction. The fit is snug, but not tight. As such, mounting it on the
ceiling is not advised, as it will almost certainly work loose from the
vibrations of people walking through the building and invariably land on your
cat's head. (Don't say I didn't warn you.)

### Base station

This is not intended to be a plug and go - users are expected
to program this for their specific home layout. The idea is that, if your
sensors all report the temperature, you can define what happens using
NodeRED. For the unfamiliar, NodeRED is a programming language using drag and
drop logic blocks, with the ability to program custom logic blocks in
JavaScript. As such, it should be pretty approachable to the novice
programmer.

**Status:** Not started

1. Hardware selection: Completed, needs schematic.
1. Firmware: Not started.
  1. RPi base uses RPiOS + NodeRed + Mosquitto MQTT broker
  1. Firmware will be a NodeRed program; example will be provided.

#### BOM

Example links are provided for clarity, in cases where the hardware is uncommon.

##### Hardware list

1. Raspberry Pi 3B+
   * RPi4 would likely work with minimal modification to these
   instructions.
1. SD card 
   * 32GB is fine.
1. RPi screw terminal expansion board with status LEDs
   * https://www.amazon.com/GeeekPi-Raspberry-Terminal-Breakout-Expansion/dp/B08GKSF1MD
   * Not strictly necessary, as one can just wire directly to the header block,
     but screw terminals ensure positive connections that won't pull out on you.
1. Some solid state relay modules
   * I bought one of
     https://www.amazon.com/SainSmart-Channel-Duemilanove-MEGA2560-MEGA1280/dp/B00ZZVQR5Q
     for each of my zones.
   * You can get ones with more or fewer relays as needed.
   * Note that you will need at least 3 for a common setup:
     * Fan
     * Cooling
     * Heating

     More advanced setups (for example, multistage heating and cooling) will
     require more contacts.
   * You can use mechanical relays if you like, as they are cheaper. However,
     they draw more current and will (eventually) mechanically wear out. The RPi
     3B+ GPIOs are spec-ed at max 16mA per channel, with 50mA aggregate.
       * This article from arrow explains it well:
         https://www.arrow.com/en/research-and-events/articles/crydom-solid-state-relays-vs-electromechanical-relays
       * Using some commonly available numbers as an example, the stated typical
         current draw for a commonly used electromechanical relay was 70mA,
         wheras it was 10mA for the SSR (though in both cases, actual measured
         current draw in situ was about an order of magnitude less).
   * Note that there is a bit of a cheat here - I assume that only one contact
     will be closed at any given time per zone - that is, only one of fan, heat,
     or cool. If this is not true, and you have enough zones, you can exceed the
     overall current rating for the board. For example, if we turn on fan and
     heat for 3 zones at the stated 10mA power consumption, we will end up at
     60mA, which is more than the allowed aggregate across all GPIO contacts.
  * 24VAC to 5V buck converter
    * I used
      https://www.amazon.com/SMAKN%C2%AE-Converter-Voltage-Supply-Waterproof/dp/B00RE6QN4U
      soldered to the power and ground pins of a USB wire.
    * The logic here is that we can power it from the 24VAC that's readily
      available at the equipment and won't have to use an additional power brick.

##### Misc supplies and equipment

1. Stranded wire for connecting screw terminals to relays.
1. Tools - strippers, small screwdrivers, etc.
1. Some lengths of furnace wire for connecting to HVAC equipment or a zone controller.

## Security notes

### WiFi

Connecting to networks with crappy (or no) security is allowed. It's your
funeral.

### SSL

#### Sensor

The default sensor firmware does not do CA certificate verification on the cert
presented by the MQTT server. This is because the common case for this
application is nodes in one's house talking to a controller via a private
network with an internal, private DNS. As a result, it is most likely that a
self signed certificate is in use here, especially since even a minimally
verified certificate such as LetsEncrypt won't work because it can't even do a
basic DNS lookup on the MQTT broker. Having that selfsigned cert be validated
would require the user to recompile the firmware with the CA cert used to sign
the server key built in. This is definitely possible, and any users who wish to
do such a thing can figure it out (look at
`examples/protocols/mqtt/ssl/main/app_main.c` and the initialization of
`mqtt_cfg` in `mqtt_start` in `main/wifi.c`). However, I'm not going to bother
with it, as even using SSL is barely commensurate with the level of risk
presented here - especially when one takes into account using a secured WiFi
network. We're at 2 levels of encryption and credentials, and I'm not going lose
any sleep over the fact that the second layer of encryption is not as strong as
it could be.

#### NodeRED / Webserver

This is just using the default selfsigned cert that's generated when Apache is
installed. 

## License

Broadly speaking, this code is licensed under the 3-Clause BSD license. However,
it incorporates various third party modules that have their own licenses, in
which case those licenses apply. If there is ever a conflict (for example, a
derived work), then the terms of that license have precedence. See the LICENSE
file for more details.

## FAQ

### **Y U NO ZIGBEE?**

Because I have WiFi coverage all over my house, and the cost is about the same.
I will grant you that battery life would be better with Zigbee, but that's not
the only consideration in play here.

### **What's all this about your data and huge coporations?**

Maybe it's because I don't like being the product. Maybe I just read too much
cyberpunk. Maybe I just am a little paranoid. Or maybe I'm just a control freak.
Take your best guess.

### **But, half the links are to Amazon!**

Yeah, I'm a hypocrite too, what's your point?