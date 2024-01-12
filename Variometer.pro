QT       += core gui sensors positioning multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    Generator.cpp \
    KalmanFilter.cpp \
    PiecewiseLinearFunction.cpp \
    main.cpp \
    mainwindow.cpp \
    readgps.cpp \
    sensormanager.cpp \
    variosound.cpp

HEADERS += \
    Generator.h \
    KalmanFilter.h \
    PiecewiseLinearFunction.h \
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
}

win32 {
    message("win32 enabled")
}


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
