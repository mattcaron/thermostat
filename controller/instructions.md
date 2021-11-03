# Controller setup instructions

Assuming one is starting with a Raspberry Pi 3 or similar...

1. Install Raspberry Pi OS on the SD card via your favorite method and boot up the system.

2. Make accounts and apply updates as normal:

       sudo apt update
       sudo apt dist-upgrade

3. Set it up however else you like. For example, I disabled wifi to save power and because I intend to just use it wired.

4. Install the various packages that we'll need:

       sudo apt install mosquitto

5. Configure mosquitto, the MQTT broker to which the sensors will send messages.

    TODO - add things here