#include <QDebug>
#include <QGuiApplication>
#include <QScreen>

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

QList< xcb_window_t > LinuxWindowCapture::listWindowsRecursive( xcb_connection_t* dpy, xcb_window_t& window )
{
  QList< xcb_window_t > windows;
  xcb_query_tree_reply_t* queryR;

  xcb_query_tree_cookie_t queryC = xcb_query_tree( dpy, window );

  if( ( queryR = xcb_query_tree_reply( dpy, queryC, NULL ) ) ) {
    xcb_window_t* children = xcb_query_tree_children( queryR );

    for( int c = 0; c < xcb_query_tree_children_length( queryR ); ++c  ) {
      windows << children[ c ];
      windows << listWindowsRecursive( dpy, children[ c ]);
    }
    free( queryR );
  }
  return windows;
}

bool LinuxWindowCapture::WindowRect() {
  if( mWindow == 0 )
    return false;

  xcb_connection_t* dpy = xcb_connect( NULL, NULL );
  if ( xcb_connection_has_error( dpy ) )
    qDebug() << "Can't open display";

  xcb_screen_t* screen = xcb_setup_roots_iterator( xcb_get_setup( dpy ) ).data;
  if( !screen )
    qDebug() << "Can't acquire screen";

  xcb_get_geometry_cookie_t geometryC = xcb_get_geometry( dpy, mWindow );
  xcb_get_geometry_reply_t* geometryR = xcb_get_geometry_reply( dpy, geometryC, NULL );

  if( geometryR ) {
    xcb_query_tree_reply_t *tree = xcb_query_tree_reply ( dpy,
                                                          xcb_query_tree ( dpy, mWindow ),
                                                          NULL );

    if( tree->parent != screen->root ){
      xcb_translate_coordinates_cookie_t
                      translateC = xcb_translate_coordinates( dpy,
                                                              mWindow,
                                                              screen->root,
                                                              geometryR->x,
                                                              geometryR->y );
      xcb_translate_coordinates_reply_t*
          translateR = xcb_translate_coordinates_reply( dpy, translateC, NULL );

      mRect.setRect( translateR->dst_x,                   
                     translateR->dst_y,
                     geometryR->width,
                     geometryR->height );

      free( translateR );
    }
    else {
      // if the parent is the screen then we can just use the coordinates directly
      mRect.setRect( geometryR->x,                   
                     geometryR->y,
                     geometryR->width,
                     geometryR->height );

    }
    free( geometryR );      
    free( tree );
    xcb_disconnect( dpy );

    return true;
  }
  xcb_disconnect( dpy );
  return false;
}

xcb_window_t LinuxWindowCapture::FindWindow( const QString& instanceName, const QString& windowClass ) {

  xcb_window_t window = static_cast< xcb_window_t > ( 0 );
  xcb_connection_t* dpy = xcb_connect( NULL, NULL );
  if ( xcb_connection_has_error( dpy ) ) { qDebug() << "Can't open display"; }

  xcb_screen_t* screen = xcb_setup_roots_iterator( xcb_get_setup( dpy ) ).data;
  if( !screen ) { qDebug() << "Can't acquire screen"; }

  xcb_window_t root = screen->root;
  QList< xcb_window_t > windows = listWindowsRecursive( dpy, root );

  foreach( const xcb_window_t& win, windows ) {

    xcb_icccm_get_wm_class_reply_t wmNameR;
    xcb_get_property_cookie_t wmClassC = xcb_icccm_get_wm_class( dpy, win );
    if ( xcb_icccm_get_wm_class_reply( dpy, wmClassC, &wmNameR, NULL ) ) {

      if( !qstrcmp( wmNameR.class_name, windowClass.toStdString().c_str() ) ||
          !qstrcmp( wmNameR.instance_name, instanceName.toStdString().c_str() ) ) {
          qDebug() << wmNameR.instance_name;
          qDebug() << wmNameR.class_name;
          window = win;
          break;
      }
    }

  }
  xcb_disconnect( dpy );
  return window;
}


bool LinuxWindowCapture::HasFocus() {
  if( mWindow ) {
    xcb_window_t focusW;
    xcb_connection_t* dpy = xcb_connect( NULL, NULL );
    if ( xcb_connection_has_error( dpy ) ) { qDebug() << "Can't open display"; }

    xcb_get_input_focus_cookie_t focusC = xcb_get_input_focus( dpy );
    xcb_get_input_focus_reply_t* focusR = xcb_get_input_focus_reply( dpy, focusC, NULL );
    focusW = focusR->focus;

    free( focusR );
    xcb_disconnect( dpy );

    return focusW == mWindow;
  }
  return false;
}

