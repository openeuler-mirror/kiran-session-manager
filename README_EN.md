# kiran-session-manager

The session management program for the KIRAN Desktop Environment.

## Build and Installation

```bash
# yum install cmake glibmm24-devel glib2-devel gtkmm30-devel systemd-devel gettext gcc-c++ dbus-daemon jsoncpp-devel kiran-log-gtk3-devel gdbus-codegen-glibmm fmt-devel gtest-devel libICE-devel libSM-devel
# mkdir build
# cd build && cmake -DCMAKE_INSTALL_PREFIX=/usr ..
# make
# make install
```

## Usage

This service is automatically launched by LightDM after selecting the KIRAN desktop environment at the login screen.The launch configuration is located at: `/usr/share/xsession/kiran.desktop`
