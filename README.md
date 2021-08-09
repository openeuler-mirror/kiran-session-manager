# KIRAN会话管理
该项目为KIRAN桌面环境的会话管理程序。

## 编译安装
```
# yum install cmake glibmm24-devel glib2-devel gtkmm30-devel systemd-devel gettext gcc-c++ dbus-daemon jsoncpp-devel kiran-log-gtk3-devel gdbus-codegen-glibmm fmt-devel gtest-devel
# mkdir build
# cd build && cmake -DCMAKE_INSTALL_PREFIX=/usr ..
# make
# make install
```

## 运行
该服务是在登陆界面选择kiran桌面环境后，由lightdm拉起，启动的配置为/usr/share/xsession/kiran.desktop