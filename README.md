# bcd-clock

This is my simple C app that drives my Binary Clock Shield for Raspberry Pi.

To use this, you'll need to have a Raspbian install built and running on your Raspberry Pi (I would recommend using the latest "Raspbian Lite" as there's no need of X-Windows, but that's just me).

You need to have installed Mike McCauley's excellent BCM2835 library first (from http://www.airspayce.com/mikem/bcm2835/). That's the only dependency.

To compile, enter: gcc -o bcd-clock bcd-clock.c -l bcm2835

To run it: ./bcd-clock [brightness]

... where [brightness] is a number between 1 and 15. It defaults to 8.

If you want your RasPi to run it on startup, then add this line near the end of your /etc/rc.local file:

/home/pi/bcd-clock 15 &

(The '&' forces it to the background.)

See my blog post on this project, here: http://danceswithferrets.org/geekblog/?p=1087

Kris.
