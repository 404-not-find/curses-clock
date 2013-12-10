curses-clock
============

A clock based on curses in C that uses fixed-sized fonts (PSF) to allow varied displays.

![screen shot of default config](/docs/clock001.png)

Usage
-----

Run `make` to generate `cclock` and then run `./cclock`.  Enjoy.

Inspirations
------------

* tablespoon's clock written with tput in shell script
* George Carlin

Build Requirements
------------------

* ncurses headers and libs (package libncurses5-dev in debian, ??? in RH)
* glib-json (package libjson-glib-dev in debian)

TODO
----

* BUG: handle window resizing
* check for json config in home directory, read it in
* write out default config on request
* error handling for unknown timezones

Acknowledgements
----------------
* https://github.com/talamus/rw-psf provided the best way to learn how to parse PSF that I have found yet.  Perl++  http://linuxgazette.net/91/loozzr.html is also informative.
* http://stackoverflow.com/questions/111928/is-there-a-printf-converter-to-print-in-binary-format made a good starting place for my byte_to_binary()
* http://www.gtkforums.com/viewtopic.php?f=3&t=178486 for how to get time out of glib
