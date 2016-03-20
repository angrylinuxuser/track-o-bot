#pragma once

#include "WindowCapture.h"

#include <QTimer>
#include <QGuiApplication>
#include <QScreen>
#include <X11/Xlib.h>

// FindWindow is quite intensive in terms of performance
#define LINUX_UPDATE_WINDOW_DATA_INTERVAL 3000 // ms

class LinuxWindowCapture : public QObject, public WindowCapture
{
Q_OBJECT

private:
  QTimer*  mTimer;
  QString   mWindowName;
  int      mWinId;
  QRect   mRect;

  static int FindWindow( const QString& name );
  static bool WindowRect( int windowId, QRect *rect );

  bool Fullscreen();

private slots:
  void Update();

public:
  LinuxWindowCapture( const QString& windowName );
  static QList<Window> listXWindowsRecursive(Display *disp, Window w);

  bool WindowFound();
  int Width();
  int Height();

  int Left();
  int Top();

  QPixmap Capture( int x, int y, int w, int h );
};
