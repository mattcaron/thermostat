# To build

1. Run `./create_venv.sh` to create the python venv. You only need to
   do this once.
2. "Dot in" setenv.sh as `. setenv.sh`. Do this every time you build.
3. Build as normal (`make -j8 flash`, etc. or just use `./flashmonitor.sh`.


Why do we need a venv?

Because the world has moved on and the modern setuptools package
doesn't have the pkg_resources package, so we need to create a venv
with the old version in order for the build to work.

The requirements.txt is based off `ESP8266_RTOS_SDK/requirements.txt`,
but setuptools is pinned to an old enough version.
