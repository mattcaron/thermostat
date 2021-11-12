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
  1. MQTT: In progress
  1. Power saving deep sleep: Not started
1. Wiring diagram: Not started
1. Case design: Not started

### Base station

This is not intended to be a plug and go - users are expected
to program this for their specific home layout. The idea is that, if your
sensors all report the temperature, you can define what happens using
NodeRED. For the unfamiliar, NodeRED is a programming language using drag and
drop logic blocks, with the ability to program custom logic blocks in
JavaScript. As such, it should be pretty approachable to the novice
programmer.

**Status:** Not started

1. Hardware selection: Completed, needs documentation and schematic.
1. Firmware: Not started.
  1. RPi base uses RPiOS + NodeRed + Some MQTT broker
  1. Firmware will be a NodeRed program; example will be provided.

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

Broadly speaking, this code is licensed under the 3-Clause BSD license.
However, it incorporates various third party modules that have their own licenses, in which case those licenses apply. If there is ever a conflict (for example, a derived work), then the terms of that license have precedence. See the LICENSE file for more details.
