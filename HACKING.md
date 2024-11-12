Building
========

For build instructions see the README.md. For coding style,
conventions, etc. check phosh's [HACKING.md].

[HACKING.md](https://gitlab.gnome.org/World/Phosh/phosh/-/blob/main/HACKING.md)

Before adding code here please check if [GNOME settings](https://source.puri.sm/pureos/packages/gnome-control-center/)
isn't the better place as this benefits mobile and non-mobile users alike.

Environment variables
---------------------

mobile settings honors the following enviroment variables that might
be useful for development:

- `MS_FORCE_OSK` : assume a certain on screen keyboard. Valid values
  are `pos`, `squeekboard` and `unknown`
