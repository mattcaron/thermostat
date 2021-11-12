# Controller setup instructions

Assuming one is starting with a Raspberry Pi 3 or similar...

1. Install Raspberry Pi OS on the SD card via your favorite method and boot up
   the system.

2. Make accounts and apply updates as normal:

       sudo apt update
       sudo apt dist-upgrade

3. Set it up however else you like. For example, I disabled wifi to save power
   and because I intend to just use it wired.

4. Install the various packages that we'll need:

       sudo apt install mosquitto

5. Configure mosquitto, the MQTT broker to which the sensors will send messages.

    1. Add a user to receive messages. Note that one can use the same user for
       all stations if one desires:

           sudo mosquitto_passwd -c /etc/mosquitto/passwd <user>

    2. Grab the example config file and gunzip it.

           cd /etc/mosquitto/conf.d/.
           sudo cp /usr/share/doc/mosquitto/examples/mosquitto.conf.gz .
           sudo gunzip mosquitto.conf.gz

    3. Edit `/etc/mosquitto/conf.d/mosquitto.conf` via your favorite method and
       go through it setting config values appropriately. It is very well
       documented in the example and most of the values are fine at their
       defaults. I note that I only set up the default listener, and set up the following:

           port 8883
           cafile /etc/mosquitto/certs/ca.crt
           certfile /etc/mosquitto/certs/crt              
           keyfile /etc/mosquitto/certs/key 

    4. Set up certs

       1. Generate certs/keys/chain files/etc. via your favorite method.
       2. Put them in `/etc/mosquitto/certs`
       3. Make sure that they are owned:growned by `mosquitto:mosquitto`.