#pragma once

#include "WindowCapture.h"

#include <QTimer>
#include <QGuiApplication>
#include <QScreen>
#include <QFile>
#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonObject>
#include <X11/Xlib.h>

// FindWindow is quite intensive in terms of performance
#define LINUX_UPDATE_WINDOW_DATA_INTERVAL 3000 // ms

class LinuxWindowCapture : public QObject, public WindowCapture
{
Q_OBJECT

private:
  QTimer*  mTimer;
  int      mWinId;
  QRect   mRect;
  QString ReadAgentAttribute( const char *attributeName ) const;

  static int FindWindow( const QString& name );
  static bool WindowRect( int windowId, QRect *rect );

  bool Fullscreen();

private slots:
  void Update();

public:
  LinuxWindowCapture();
  static QList<Window> listXWindowsRecursive(Display *disp, Window w);

  bool WindowFound();
  int Width();
  int Height();

  int Left();
  int Top();

  QPixmap Capture( int x, int y, int w, int h );
  bool Focus();
};

