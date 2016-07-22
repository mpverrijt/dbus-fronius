# Application version and revision
VERSION = 1.0.9

# suppress the mangling of va_arg has changed for gcc 4.4
QMAKE_CXXFLAGS += -Wno-psabi

# gcc 4.8 and newer don't like the QOMPILE_ASSERT in qt
QMAKE_CXXFLAGS += -Wno-unused-local-typedefs

# Add more folders to ship with the application here
unix {
    bindir = $$(bindir)
    DESTDIR = $$(DESTDIR)
    isEmpty(bindir) {
        bindir = /usr/local/bin
    }
    INSTALLS += target
    target.path = $${DESTDIR}$${bindir}
}

# Note: we need the script module to parse JSON fragments as part of the
# fronius solar API.
QT += core network dbus script
QT -= gui

TARGET = dbus-fronius
CONFIG += console
CONFIG -= app_bundle
DEFINES += VERSION=\\\"$${VERSION}\\\"

TEMPLATE = app

include(src/json/json.pri)
include(ext/qslog/QsLog.pri)

VELIB_INC = ext/velib/inc/velib/qt
VELIB_SRC = ext/velib/src/qt

INCLUDEPATH += \
    ext/qslog \
    ext/velib/inc \
    ext/velib/src/qt \
    src \
    src/modbus_tcp_client

SOURCES += \
    $$VELIB_SRC/v_busitem.cpp \
    $$VELIB_SRC/v_busitems.cpp \
    $$VELIB_SRC/v_busitem_adaptor.cpp \
    $$VELIB_SRC/v_busitem_private_cons.cpp \
    $$VELIB_SRC/v_busitem_private_prod.cpp \
    $$VELIB_SRC/v_busitem_proxy.cpp \
    src/main.cpp \
    src/froniussolar_api.cpp \
    src/inverter.cpp \
    src/power_info.cpp \
    src/inverter_updater.cpp \
    src/inverter_gateway.cpp \
    src/local_ip_address_generator.cpp \
    src/settings.cpp \
    src/dbus_fronius.cpp \
    src/dbus_bridge.cpp \
    src/dbus_inverter_bridge.cpp \
    src/dbus_settings_bridge.cpp \
    src/inverter_settings.cpp \
    src/dbus_inverter_settings_bridge.cpp \
    src/dbus_gateway_bridge.cpp \
    src/fronius_data_processor.cpp \
    src/fronius_device_info.cpp \
    src/v_bus_node.cpp \
    src/inverter_mediator.cpp

HEADERS += \
    $$VELIB_SRC/v_busitem_adaptor.h \
    $$VELIB_SRC/v_busitem_private_cons.h \
    $$VELIB_SRC/v_busitem_private_prod.h \
    $$VELIB_SRC/v_busitem_private.h \
    $$VELIB_SRC/v_busitem_proxy.h \
    $$VELIB_INC/v_busitem.h \
    $$VELIB_INC/v_busitems.h \
    src/froniussolar_api.h \
    src/inverter.h \
    src/power_info.h \
    src/inverter_updater.h \
    src/inverter_gateway.h \
    src/local_ip_address_generator.h \
    src/settings.h \
    src/dbus_fronius.h \
    src/dbus_bridge.h \
    src/dbus_inverter_bridge.h \
    src/dbus_settings_bridge.h \
    src/inverter_settings.h \
    src/dbus_inverter_settings_bridge.h \
    src/dbus_gateway_bridge.h \
    src/defines.h \
    src/fronius_data_processor.h \
    src/fronius_device_info.h \
    src/v_bus_node.h \
    src/inverter_mediator.h \
    src/velib/velib_config_app.h \
    src/modbus_tcp_client/modbus_tcp_client.h \
    src/inverter_modbus_updater.h

DISTFILES += \
	../README.md
