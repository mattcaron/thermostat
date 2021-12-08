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

```
git clone --recursive https://github.com/mattcaron/thermostat.git
```

This will get this repo and all the submodules uses (such as the Espressif repositories).

## Components

### Thermometer puck sensors:

This is intended to be pretty turnkey. Build it, flash it, configure it,
stick it on the wall. It will send MQTT messages to your broker (see below)
which you then consume in NodeRED (also below) and make it do what you want.

**Status:** In progress. See the `sensors` subdirectory.

**Roadmap:**
1. Hardware selection: Completed, needs documentation and schematic.
1. Firmware: In progress
  1. Console: Done
  1. Config: Done
  1. OneWire to DS18B20: Done
  1. WiFi refactor: Done
  1. MQTT: Done
  1. Power saving deep sleep: Not started
1. Wiring diagram: Not started
1. Case design: In progress

**TODO:**

1. Implement CLI command completion hinting (see iperf example code)

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
   * https://www.mouser.com/ProductDetail/?qs=7H2Jq%252ByxpJKpIDCbiq4lfg%3D%3D

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
   * Note that there is a bit of a cheat here - I assume that only one
     contact will be closed at any given time per zone - that is, only one of
     fan, heat, or cool. If this is not true, and you have enough zones, you can
     exceed the overall current rating for the board. For example, if we turn on
     fan and heat for 3 zones at the stated 10mA power consumption, we will end
     up at 60mA, which is more than the allowed aggregate across all GPIO
     contacts.
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

The default firmware does not do CA certificate verification on the cert
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