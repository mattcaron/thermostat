# Controller setup instructions

Assuming one is starting with a Raspberry Pi 3 or similar...

1. Install Raspberry Pi OS Lite on the SD card via your favorite
   method and boot up the system.

1. Make a user. From here on out, the docs will assume your user is `alice`.
   Replace as appropriate.

    * **Note:** In earlier Rasberry Pi OS builds, there was a default `pi` user
      which some software (such as Node Red) assumed would be there. In Bullseye
      and later, this seems to be the user created above, but this may not
      always be true. See <https://www.raspberrypi.com/news/raspberry-pi-bullseye-update-april-2022/>.

1. Apply updates as normal:

       sudo apt update
       sudo apt dist-upgrade

1. Set it up however else you like. For example, I disabled wifi to save power
   and because I intend to just use it wired.

1. Set up NodeRED / Apache / etc.

   1. Install necessary dependencies:

          sudo apt install nodered apache2 libapache2-mod-authnz-external

   1. Generate a PEM cert/key pair via your favorite method and put them in a
      useful place. The config below assumes they will be in `/etc/ssl/private`
      and be called `apache.pem` and `apache.key`.

   1. Create a config file akin to the following in
      `/etc/apache2/sites-available/thermostat.conf`. Make sure to replace
      things for your server, email, etc. as appropriate.

          <VirtualHost _default_:80>
              ServerName thermostat
              ServerAdmin user@domain.com

              Redirect /nodered  https://thermostat/nodered
              Redirect /editor  https://thermostat/editor

              ProxyPass / http://localhost:1880/nodered/ui/
              ProxyPassReverse / http://localhost:1880/nodered/ui/
          </VirtualHost>

          <VirtualHost _default_:443>
              ServerName stairs
              ServerAdmin matt@mattcaron.net

              SSLEngine on
              SSLCertificateFile    /etc/ssl/private/apache.pem
              SSLCertificateKeyFile /etc/ssl/private/apache.key

              # Standard SSL protocol adustments for IE
              BrowserMatch "MSIE [2-6]" \
                           nokeepalive ssl-unclean-shutdown \
                           downgrade-1.0 force-response-1.0
              BrowserMatch "MSIE [17-9]" ssl-unclean-shutdown

              AddExternalAuth pwauth /usr/sbin/pwauth
              SetExternalAuthMethod pwauth pipe

              <Location "/nodered">
                  AuthType Basic
                  AuthName "Login"
                  AuthBasicProvider external
                  AuthExternal pwauth
                  Require valid-user
              </Location>

              SSLProxyEngine On
              ProxyPreserveHost On
              ProxyRequests Off

              ProxyPass /nodered/comms  ws://localhost:1880/nodered/comms

              ProxyPass /nodered/comms http://localhost:1880/nodered/comms
              ProxyPassReverse /nodered/comms http://localhost:1880/nodered/comms

              ProxyPass /nodered http://localhost:1880/nodered
              ProxyPassReverse /nodered http://localhost:1880/nodered

              ProxyPass /editor http://localhost:1880/nodered
              ProxyPassReverse /editor http://localhost:1880/nodered

              ProxyPass / http://localhost:1880/nodered/ui/
              ProxyPassReverse / http://localhost:1880/nodered/ui/

          </VirtualHost>

   1. Edit the NodeRed config file (`/home/alice/.node-red/settings.js`), and make
      the following changes:

      1. Uncomment the `uiHost: "127.0.0.1",` so it only listens on localhost
         (the rest is handled by the Apache proxy, above).

      1. Set `httpRoot: '/nodered',`

   1. Enable the site above, disable the default one, and reload the config.

          sudo service nodered start
          sudo a2enmod ssl authnz_external proxy proxy_http proxy_wstunnel
          sudo a2ensite thermostat
          sudo a2dissite 000-default
          sudo service apache2 restart

   1. (Optional) Add some useful NodeRed things.

       **NOTE:** Do all these in the `~alice/.node-red` directory

      1. Add UI Components:

             npm install node-red-dashboard
             npm install node-red-contrib-flogger
             npm install node-red-node-email
             npm install node-red-contrib-coap

   1. Once that's all done, you'll need to restart nodered:

          sudo service nodered restart

   1. And make sure it starts on boot

          sudo systemctl enable nodered

   1. Allow for local temp sensing

      1. Add the following to `/etc/rc.local` (before the `exit`), making sure
         to set the `gpiopin` to whatever GPIO you attached any local sensors.
         Note that multiple devices can be created by repeating this command:

             dtoverlay w1-gpio gpiopin=14 pullup=0

      1. The device will be something like `/sys/bus/w1/devices/28-012062f7a914`
         * you'll need to look around and find it, each has a different ID.

      1. Add bc to make it easier to do math from the CLI

             sudo apt install bc

      1. And this shell script will read it out and print it on STDOUT so we can
         read it in to NodeRED. Make sure to change the `cat` command to the
         device attached to your system.

             #!/bin/sh

             temp_c=`cat /sys/bus/w1/devices/28-012062f7a914/temperature`
             echo "scale=1; $temp_c/1000*1.8 + 32" | bc

   1. Set up system email forwarding (assuming you want to get emails from the
      watchdog when it reboots the system):

      1. Install it:

             sudo apt install ssmtp

      1. And then configure `/etc/ssmtp/ssmtp.conf` appropriately.

   1. Enable the watchdog timer.

      This is so that, if the hardware locks up, it will get rebooted.

      1. Enable it in the devicetree.
         1. Edit `/boot/config.txt`
         1. Find the `dtparam` lines that enable `spi` and `i2c`.
         1. Add the following line:

                dtparam=watchdog=on

      1. Install the `watchdog` package:

             sudo apt install watchdog

      1. Configure it:
         1. Edit `/etc/watchdog.conf`
         1. Uncomment the following lines:

                watchdog-device         = /dev/watchdog
                max-load-1              = 24
                max-load-5              = 18
                max-load-15             = 12
                min-memory              = 1
                allocatable-memory      = 1

         1. And this one needs to be uncommented and changed:

                watchdog-timeout        = 15

         1. The rest should be fine at the defaults.

      1. Enable it:

             sudo systemctl enable watchdog

      1. Reboot

1. Make sure to give it either a static IP or a static DHCP lease and add that
   IP to your local network DNS lookup. The reason here is that the sensors need
   to be able to get to it to submit their temperature information. If they are
   using static IPs, they will connect to that IP, so it better not change. If
   you have it use a hostname, it will do a DNS lookup, and that could change,
   but if you reboot the router, then it may not end up resolving until the next
   time the controller renews its lease (unless you go reboot it). In that
   scenario, you run the risk of the sensors not being able to connect to the
   controller for however long the DHCP lease has remaining. If it's a static
   lease and associated DNS entry, however, if will keep the old IP which will
   be valid (because the DHCP server hands out the same IP every time) and the
   hostname will resolve, because it's in a static lookup table rather than
   being dynamically updated by DHCP registrations.
