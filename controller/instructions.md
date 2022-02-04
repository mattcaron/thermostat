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
       4. And that they're group readable:

              sudo chmod g+r /etc/mosquitto/certs/* 

       5. At that the user `pi` is in the `mosquitto` group so NodeRed (which
          runs as `pi`) can read them.

              sudo usermod -a -G mosquitto pi

6. Set up NodeRED / Apache / etc.

   1. Install necessary dependencies:

          sudo apt install nodered apache2 libapache2-mod-authnz-external

   2. Generate a PEM cert/key pair via your favorite method and put them in a
      useful place. The config below assumes they will be in `/etc/ssl/private`
      and be called `apache.pem` and `apache.key`.

   3. Create a config file akin to the following in
      `/etc/apache2/sites-available/thermostat.conf`. Make sure to replace
      things for your server, email, etc. as appropriate.

          <VirtualHost _default_:80>
              ServerName thermostat
              ServerAdmin user@domain.com

              Redirect /  https://thermostat/
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

   4. Edit the NodeRed config file (`/home/pi/.node-red/settings.js`), and make
      the following changes:

      1. Uncomment the `uiHost: "127.0.0.1",` so it only listens on localhost
         (the rest is handled by the Apache proxy, above).

      2. Set `httpRoot: '/nodered',`

   5. Enable the site above, disable the default one, and reload the config.

          sudo service nodered start
          sudo a2enmod ssl authnz_external proxy proxy_http proxy_wstunnel
          sudo a2ensite thermostat
          sudo a2dissite 000-default
          sudo service apache2 restart

   6. (Optional) Add some useful NodeRed things.

       **NOTE:** Do all these in the `~/pi/.node-red` directory

      1. UI Components:

             npm install node-red-dashboard
             npm install node-red-contrib-flogger


       Once that's all done, you'll need to restart nodered:

          sudo service nodered restart