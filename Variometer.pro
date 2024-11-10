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

# Add required frameworks for iOS
    LIBS += -framework CoreMotion
    LIBS += -framework CoreLocation

    # Add required capabilities
    QMAKE_APPLE_TARGETED_DEVICE_FAMILY = 1,2
    QMAKE_APPLE_DEVICE_ARCHS = arm64

    # Add background mode capabilities
    QMAKE_MAC_XCODE_SETTINGS += background_modes
    background_modes.name = UIBackgroundModes
    background_modes.value = location

    QMAKE_APPLE_TARGETED_DEVICE_FAMILY = 1,2 # Support both iPhone and iPad

    # Enable background location updates
    QMAKE_MAC_XCODE_SETTINGS += background_location
    background_location.name = UIBackgroundModes
    background_location.value = location

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
