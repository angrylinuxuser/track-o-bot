#ifndef LINUXWINDOWCAPTURE_H
#define LINUXWINDOWCAPTURE_H

#include "WindowCapture.h"
#include <xcb/xcb.h>

class LinuxWindowCapture : public QObject, public WindowCapture
{
Q_OBJECT

private:
  xcb_window_t mWindow;
  QRect   mRect;

  xcb_window_t FindWindow( const QString& wmName, const QString& wmClass );
  QList< xcb_window_t > listWindowsRecursive( xcb_connection_t* dpy, xcb_window_t& window );
  bool WindowRect();

public:
  LinuxWindowCapture();

  bool WindowFound();
  int Width();
  int Height();

  int Left();
  int Top();

  QPixmap Capture( int x, int y, int w, int h );
  bool HasFocus();
};

#endif // LINUXWINDOWCAPTURE_H
