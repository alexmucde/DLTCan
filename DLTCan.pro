QT       += core gui serialport network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    dltcan.cpp \
    dltminiserver.cpp \
    main.cpp \
    dialog.cpp \
    settingsdialog.cpp

HEADERS += \
    dialog.h \
    dltcan.h \
    dltminiserver.h \
    settingsdialog.h \
    version.h

FORMS += \
    dialog.ui \
    settingsdialog.ui

TRANSLATIONS += \
    DLTMultimeter_en_DE.ts

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    .gitignore \
    DLTRelais_en_DE.ts \
    LICENSE \
    README.md \
    build_windows_qt5_MSVC.bat \
    build_windows_qt5_MSVC.bat
