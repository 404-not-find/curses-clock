curses-clock
============

A clock based on curses in C

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

* fontpath
* parse PSF
* print tall numbers and letters
* multiple timezones
* color
* BUG: handle window resizing

Acknowledgements
----------------
* https://github.com/talamus/rw-psf provided the best way to learn how to parse PSF that I have found yet.  Perl++
