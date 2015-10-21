TARGET = Track-o-Bot
VERSION = 0.6.2

CONFIG += qt precompile_header debug_and_release c++11
QT += core widgets network

DESTDIR = build
OBJECTS_DIR = tmp
MOC_DIR = tmp
RCC_DIR = tmp
UI_DIR = tmp

PRECOMPILED_HEADER = src/Local.h
HEADERS = src/Local.h \
          src/ui/Window.h \
          src/ui/SettingsTab.h \
          src/ui/AccountTab.h \
          src/ui/LogTab.h \
          src/ui/AboutTab.h \
          src/Core.h \
          src/Logger.h \
          src/WebProfile.h \
          src/HearthstoneLogWatcher.h \
          src/HearthstoneLogTracker.h \
          src/MLP.h \
          src/RankClassifier.h \
          src/Settings.h \
          src/ResultTracker.h \
          src/ResultQueue.h \
          src/Trackobot.h

SOURCES = src/Main.cpp \
          src/Hearthstone.cpp \
          src/WebProfile.cpp \
          src/ui/Window.cpp \
          src/ui/SettingsTab.cpp \
          src/ui/AccountTab.cpp \
          src/ui/LogTab.cpp \
          src/ui/AboutTab.cpp \
          src/Core.cpp \
          src/Logger.cpp \
          src/Autostart.cpp \
          src/HearthstoneLogWatcher.cpp \
          src/HearthstoneLogTracker.cpp \
          src/MLP.cpp \
          src/RankClassifier.cpp \
          src/Settings.cpp \
          src/ResultTracker.cpp \
          src/ResultQueue.cpp \
          src/Local.cpp \
          src/Trackobot.cpp

FORMS   = src/ui/MainWindow.ui \
          src/ui/SettingsWidget.ui \
          src/ui/AccountWidget.ui \
          src/ui/LogWidget.ui \
          src/ui/AboutWidget.ui 

DEFINES += VERSION=\\\"$$VERSION\\\"

RESOURCES += resources.qrc


CONFIG(debug, debug|release): DEFINES += _DEBUG

mac {
  QT += macextras

  DEFINES += PLATFORM=\\\"mac\\\"

  HEADERS += src/OSXWindowCapture.h src/OSXLocal.h
  SOURCES += src/OSXWindowCapture.cpp

  # thanks qt for forcing me to do this stuff
  INCLUDEPATH += "\ -F/Library/Frameworks" 
  LIBS += -framework ApplicationServices -F/Library/Frameworks -framework Sparkle -framework AppKit

  OBJECTIVE_SOURCES += \
    src/SparkleUpdater.mm \
    src/CocoaInitializer.mm \
    src/OSXLocal.mm

  ICON = icons/logo.icns

  CONFIG(release, debug|release) {
    QMAKE_INFO_PLIST = Info.plist.app
    QMAKE_POST_LINK += /usr/libexec/PlistBuddy -c \"Set :CFBundleShortVersionString $${VERSION}\" $${DESTDIR}/$${TARGET}.app/Contents/Info.plist;
    QMAKE_POST_LINK += /usr/libexec/PlistBuddy -c \"Set :CFBundleVersion $${VERSION}\" $${DESTDIR}/$${TARGET}.app/Contents/Info.plist;
  }
}

win32 {
  QT += winextras

  CONFIG += embed_manifest_exe

  DEFINES += PLATFORM=\\\"win32\\\"

  INCLUDEPATH += . \
                 ../WinSparkle/include

  SOURCES += src/WinWindowCapture.cpp \
             src/WinSparkleUpdater.cpp

  DEFINES += _CRT_SECURE_NO_WARNINGS

  LIBS += user32.lib Gdi32.lib shell32.lib
  LIBS += -L../WinSparkle/Release

  QMAKE_PRE_LINK = ruby dist/win/patch_rc.rb
}

unix {
    DEFINES += PLATFORM=\\\"linux\\\"
    HEADERS += src/LinuxWindowCapture.h
    SOURCES += src/LinuxWindowCapture.cpp
    RESOURCES = linux.qrc
    LIBS += -lXext -lX11 -L/usr/lib/x86_64-linux-gnu/
    CONFIG += link_pkgconfig
    PKGCONFIG += x11
    isEmpty(PREFIX){
        PREFIX = /usr/local
    }
    
    greaterThan(QT_MAJOR_VERSION, 4) {
        QT += widgets
    } else {
    	QT += gui
    }
    
    desktop.path = $$PREFIX/share/applications
    desktop.files += \
        assets/track-o-bot.desktop
    target.path = $$PREFIX/bin
    icon.path = $$PREFIX/share/icons/hicolor/256x256/apps
    icon.files += icons/Track-o-Bot.png
    INSTALLS += target desktop icon
}

