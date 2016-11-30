#include <QGuiApplication>
#include <QScreen>
#include <QX11Info>
#include "LinuxWindowCapture.h"
#include <xcb/xcb_icccm.h>

#define WM_WINDOW_INSTANCE "Hearthstone.exe"
#define WM_CLASS "Hearthstone.exe"

LinuxWindowCapture::LinuxWindowCapture()
  : mWindow( 0 )
{
}

bool LinuxWindowCapture::WindowFound() {
  if ( mWindow == 0 )
    mWindow = FindWindow( WM_WINDOW_INSTANCE, WM_CLASS );

  if ( mWindow != 0 && !WindowRect() )
    mWindow = 0;

  return mWindow != 0;
}

int LinuxWindowCapture::Width() {
  return mRect.width();
}

int LinuxWindowCapture::Height() {
  return mRect.height();
}

int LinuxWindowCapture::Left() {
  return mRect.x();
}

int LinuxWindowCapture::Top() {
  return mRect.y();
}

QPixmap LinuxWindowCapture::Capture( int x, int y, int w, int h ) {
    LOG("Capturing window: %d, %d, %d, %d", x, y, h, w );
    QScreen *screen = QGuiApplication::primaryScreen();
    QPixmap pixmap = screen->grabWindow( mWindow, x, y, w, h );
  return pixmap;
}

QList< xcb_window_t > LinuxWindowCapture::listWindowsRecursive( const xcb_window_t& window )
{
  QList< xcb_window_t > windows;
  xcb_query_tree_reply_t* queryR;
  xcb_connection_t* xcbConn = QX11Info::connection();

  xcb_query_tree_cookie_t queryC = xcb_query_tree( xcbConn, window );

  if( ( queryR = xcb_query_tree_reply( xcbConn, queryC, NULL ) ) ) {
    xcb_window_t* children = xcb_query_tree_children( queryR );

    for( int c = 0; c < xcb_query_tree_children_length( queryR ); ++c  ) {
      windows << children[ c ];
      windows << listWindowsRecursive( children[ c ]);
    }
    free( queryR );
  }
  return windows;
}

bool LinuxWindowCapture::WindowRect() {
  if( mWindow == 0 )
    return false;

  xcb_connection_t* xcbConn = QX11Info::connection();

  xcb_get_geometry_cookie_t geometryC = xcb_get_geometry( xcbConn, mWindow );
  xcb_get_geometry_reply_t* geometryR = xcb_get_geometry_reply( xcbConn, geometryC, NULL );

  if( !geometryR ) return false;

  // assume the parent window is the screen then we can just use the coordinates directly
  mRect.setRect( geometryR->x, geometryR->y, geometryR->width, geometryR->height );
  free( geometryR );

  xcb_query_tree_reply_t* treeR = xcb_query_tree_reply( xcbConn, xcb_query_tree( xcbConn, mWindow ), NULL );
  if ( !treeR ) return false;

  // if the parent isn't screen translate coords
  if ( treeR->parent != QX11Info::appRootWindow() ) {
    free( treeR );
    xcb_translate_coordinates_cookie_t
                    translateC = xcb_translate_coordinates( xcbConn,
                                                            mWindow,
                                                            QX11Info::appRootWindow(),
                                                            mRect.x(),
                                                            mRect.y() );
    xcb_translate_coordinates_reply_t*
                    translateR = xcb_translate_coordinates_reply( xcbConn, translateC, NULL );
    if ( !translateR ) return false;

    mRect.setTopLeft( QPoint( translateR->dst_x, translateR->dst_y ) );
    free( translateR );
  }
  else{
    free(treeR);
  }
  return true;
}

xcb_window_t LinuxWindowCapture::FindWindow( const QString& instanceName, const QString& windowClass ) {

  xcb_window_t winID = static_cast< xcb_window_t > ( 0 );
  QList< xcb_window_t > windows = listWindowsRecursive( static_cast< xcb_window_t >( QX11Info::appRootWindow() ) );

  xcb_connection_t* xcbConn = QX11Info::connection();
  foreach( const xcb_window_t& win, windows ) {
    xcb_icccm_get_wm_class_reply_t wmNameR;
    xcb_get_property_cookie_t wmClassC = xcb_icccm_get_wm_class( xcbConn, win );
    if ( xcb_icccm_get_wm_class_reply( xcbConn, wmClassC, &wmNameR, NULL ) ) {

      if( !qstrcmp( wmNameR.class_name, windowClass.toStdString().c_str() ) ||
          !qstrcmp( wmNameR.instance_name, instanceName.toStdString().c_str() ) ) {
          winID = win;
          break;
      }
    }
  }
  return winID;
}


bool LinuxWindowCapture::HasFocus() {
  if( mWindow ) {
    xcb_window_t focusW;
    xcb_connection_t* xcbConn = QX11Info::connection();
    xcb_get_input_focus_cookie_t focusC = xcb_get_input_focus( xcbConn );
    xcb_get_input_focus_reply_t* focusR = xcb_get_input_focus_reply( xcbConn, focusC, NULL );
    focusW = focusR->focus;

    free( focusR );
    return focusW == mWindow;
  }
  return false;
}

