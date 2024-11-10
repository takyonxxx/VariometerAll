QT       += core gui sensors positioning multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

DEFINES += QT_POSITIONING_IOS    # For iOS-specific positioning

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    KalmanFilter.cpp \
    main.cpp \
    mainwindow.cpp \
    readgps.cpp \
    sensormanager.cpp \
    variosound.cpp

HEADERS += \
    KalmanFilter.h \
    mainwindow.h \
    readgps.h \
    sensormanager.h \
    utils.h \
    variosound.h

FORMS += \
    mainwindow.ui

macos {
    message("macx enabled")

    QMAKE_INFO_PLIST = ./macos/Info.plist
    QMAKE_ASSET_CATALOGS = $$PWD/macos/Assets.xcassets
    QMAKE_ASSET_CATALOGS_APP_ICON = "AppIcon"   
 }

ios {
    message("ios enabled")

    QMAKE_INFO_PLIST = ./ios/Info.plist
    QMAKE_ASSET_CATALOGS = $$PWD/ios/Assets.xcassets
    QMAKE_ASSET_CATALOGS_APP_ICON = "AppIcon"

    # Required frameworks for iOS
    LIBS += -framework CoreLocation
    LIBS += -framework CoreMotion

    # Make sure the positioning plugin is loaded
    QTPLUGIN.position = qtposition_cl
    QT += positioning-private

    # Device configuration
    QMAKE_APPLE_TARGETED_DEVICE_FAMILY = 1,2  # 1=iPhone, 2=iPad
    QMAKE_APPLE_DEVICE_ARCHS = arm64

    # Background modes
    QMAKE_MAC_XCODE_SETTINGS += background_modes
    background_modes.name = UIBackgroundModes
    background_modes.value = location

    # Objective-C sources
    OBJECTIVE_SOURCES += LocationPermission.mm
    HEADERS += LocationPermission.h
}

win32 {
    message("win32 enabled")
}


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

contains(ANDROID_TARGET_ARCH,armeabi-v7a) {
    ANDROID_PACKAGE_SOURCE_DIR = \
        $$PWD/android
}

RESOURCES += \
    resources.qrc
