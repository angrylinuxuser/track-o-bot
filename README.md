# Track-o-Bot GNU/Linux port

## Build Dependencies

* Install Qt5.5+ with your distribution package manager (apt, etc...)

You also need development packages for:

* pkg-config
* qt5-base
* qt5-x11extras
* mesa
* xcb
* xcb-icccm

on Ubuntu this should install all required development packages:

```
sudo apt-get install pkg-config build-essential qt5-default qtbase5-dev libqt5x11extras5-dev libxcb1-dev libxcb-icccm4-dev
```

## Build Instructions

```
qmake
make
```

The resulting binary can be found in the ``build`` subfolder.

## Install Instructions

```
sudo make install
```

Default install prefix is /usr/local. You can change it by adding PREFIX argument to qmake command eg:

```
qmake PREFIX=/usr
```

## Contributing

Feel free to submit pull requests, suggest new ideas and discuss issues. Track-o-Bot is about simplicity and usability. Only features which benefit all users will be considered.

## License

GNU Lesser General Public License (LGPL) Version 2.1.

See [LICENSE](LICENSE).

