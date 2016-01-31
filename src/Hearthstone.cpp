#include "Hearthstone.h"

#include <QFile>
#include <QDesktopServices>
#include <QSettings>
#include <QTextStream>
#include <QRegExp>

#include <QJsonDocument>
#include <QJsonObject>

#ifdef Q_OS_MAC
#include "OSXWindowCapture.h"
#elif defined Q_OS_WIN
#include "WinWindowCapture.h"
#include "Shlobj.h"
#elif defined Q_OS_LINUX
#include <math.h>
#include "LinuxWindowCapture.h"
#endif

DEFINE_SINGLETON_SCOPE( Hearthstone );

Hearthstone::Hearthstone()
 : mCapture( NULL ), mGameRunning( false )
{
#ifdef Q_OS_MAC
    LOG("OS X host");
  mCapture = new OSXWindowCapture( WindowName() );
#elif defined Q_OS_WIN
    LOG("Windows host");
  mCapture = new WinWindowCapture( WindowName() );
#elif defined Q_OS_LINUX
 LOG("GNU/Linux host");
 mCapture = new LinuxWindowCapture ( WindowName() );
#endif

  // On OS X, WindowFound is quite CPU intensive
  // Starting time for HS is also long
  // So just check only once in a while
  mTimer = new QTimer( this );
  connect( mTimer, &QTimer::timeout, this, &Hearthstone::Update );
  mTimer->start( 5000 );
}

Hearthstone::~Hearthstone() {
  if( mCapture != NULL )
    delete mCapture;
}

void Hearthstone::Update() {
  bool isRunning = mCapture->WindowFound();

  if( mGameRunning != isRunning ) {
    mGameRunning = isRunning;

    if( isRunning ) {
      LOG( "Hearthstone is running" );
      emit GameStarted();
    } else {
      LOG( "Hearthstone stopped" );
      emit GameStopped();
    }
  }
}

QString Hearthstone::ReadAgentAttribute( const char *attributeName ) const {
    LOG("HS ReadAgentAttribute: %s", attributeName);
#ifdef Q_OS_MAC
  QString path = "/Users/Shared/Battle.net/Agent/agent.db";
#elif defined Q_OS_WIN
  wchar_t buffer[ MAX_PATH ];
  SHGetSpecialFolderPathW( NULL, buffer, CSIDL_COMMON_APPDATA, FALSE );
  QString programData = QString::fromWCharArray( buffer );
  QString path = programData + "\\Battle.net\\Agent\\agent.db";
#elif defined Q_OS_LINUX
  QString homeLocation = QStandardPaths::writableLocation( QStandardPaths::HomeLocation );
  QString path = homeLocation + "/.Hearthstone/agent.db";
  if(QFile(path).exists()){
    LOG("HS Agent DB file (agent.db): FOUND");
  }
  else{
      LOG("HS Agent DB file (agent.db): NOT FOUND");
  }
#endif

  QFile file( path );
  if( !file.open( QIODevice::ReadOnly | QIODevice::Text ) ) {
    ERR( "Couldn't open %s (%d)", qt2cstr( path ), file.error() );
    return "";
  }

  QString contents = file.readAll();
  QJsonDocument doc = QJsonDocument::fromJson( contents.toUtf8() );
  QJsonObject root = doc.object();

  QJsonObject hs = root["/game/hs_beta"].toObject()["resource"].toObject()["game"].toObject();
  return hs[ QString( attributeName ) ].toString();
}

bool Hearthstone::GameRunning() const {
  return mGameRunning;
}

#ifdef Q_OS_WIN
inline float roundf( float x ) {
   return x >= 0.0f ? floorf( x + 0.5f ) : ceilf( x - 0.5f );
}
#endif

bool Hearthstone::CaptureWholeScreen( QPixmap *screen ) {
  *screen = mCapture->Capture( 0, 0, Width(), Height() );
  return true;
}

QPixmap Hearthstone::Capture( int canvasWidth, int canvasHeight, int cx, int cy, int cw, int ch )
{
  UNUSED_ARG( canvasWidth );

  int x, y, w, h;

  int windowHeight = mCapture->Height();
  float scale = windowHeight / float( canvasHeight );

  x = roundf( cx * scale );
  y = roundf( cy * scale );

  w = roundf( cw * scale );
  h = roundf( ch * scale );

  return mCapture->Capture( x, y, w, h );
}

void Hearthstone::SetWindowCapture( WindowCapture *windowCapture ) {
  if( mCapture != NULL )
    delete mCapture;

  mCapture = windowCapture;
}

void Hearthstone::EnableLogging() {
  QString path = LogConfigPath();
  QFile file( path );

  bool logModified = false;

  // Read file contents
  QString contents;
  if( file.exists() ) {
    file.open( QIODevice::ReadOnly | QIODevice::Text );
    QTextStream in( &file );
    contents = in.readAll();
    file.close();
  }


  // Check what modules we have to activate
  QStringList modulesToActivate;
  for( int i = 0; i < NUM_LOG_MODULES; i++ ) {
    const char *moduleName = LOG_MODULE_NAMES[ i ];
    QString moduleLine = QString( "[%1]" ).arg( moduleName ) ;
    if( !contents.contains( moduleLine ) ) {
      contents += "\n";
      contents += moduleLine + "\n";
      contents += "LogLevel=1\n";
      contents += "FilePrinting=true\n";

      DBG( "Activate module %s", moduleName );
      logModified = true;
    }
  }

  QRegExp regexEnabledConsolePrinting( "ConsolePrinting\\s*=\\s*true",
      Qt::CaseInsensitive );
  QRegExp regexDisabledFilePrinting( "FilePrinting\\s*=\\s*false",
      Qt::CaseInsensitive );
  if( contents.contains( regexEnabledConsolePrinting ) ||
      contents.contains( regexDisabledFilePrinting ) )
  {
    contents.replace( regexEnabledConsolePrinting, "FilePrinting=true" );
    contents.replace( regexDisabledFilePrinting, "FilePrinting=true" );

    DBG( "FilePrinting enabled" );
    logModified = true;
  }

  // Finally write updated log.config
  if( logModified ) {
    DBG( "Log modified. Write new version" );

    if( !file.open( QIODevice::WriteOnly | QIODevice::Text ) ) {
      ERR( "Couldn't create file %s", qt2cstr( path ) );
    } else {
      QTextStream out( &file );
      out << contents;
    }
  }

  // Notify about restart if game is running
  Update();
  if( GameRunning() && logModified ) {
    emit GameRequiresRestart();
  }
}

void Hearthstone::DisableLogging() {
  QFile file( LogConfigPath() );
  if( file.exists() ) {
    file.remove();
    LOG( "Ingame log deactivated." );
  }
}

QString Hearthstone::LogConfigPath() const {
#ifdef Q_OS_MAC
  QString homeLocation = QStandardPaths::standardLocations( QStandardPaths::HomeLocation ).first();
  QString configPath = homeLocation + "/Library/Preferences/Blizzard/Hearthstone/log.config";
#elif defined Q_OS_WIN
  wchar_t buffer[ MAX_PATH ];
  SHGetSpecialFolderPathW( NULL, buffer, CSIDL_LOCAL_APPDATA, FALSE );
  QString localAppData = QString::fromWCharArray( buffer );
  QString configPath = localAppData + "/Blizzard/Hearthstone/log.config";
#elif defined Q_OS_LINUX
  QString homeLocation = QStandardPaths::writableLocation( QStandardPaths::HomeLocation );
  QString configPath = homeLocation + "/.Hearthstone/log.config";
#endif
  if(QFile(configPath).exists()){
      LOG("HS config file (log.config): FOUND");
  }
  else{
      LOG("HS config file (log.config): NOT FOUND");
  }
  return configPath;
}

QString Hearthstone::LogPath( const QString& fileName ) const {
  static QString hsPath;

#ifdef Q_OS_LINUX
    QString homeLocation = QStandardPaths::writableLocation( QStandardPaths::HomeLocation );
    QString hsPathLnx = homeLocation + "/.Hearthstone";
    if( hsPathLnx.isEmpty() ) {
      ERR( "Hearthstone path not found" );
      return QString();
    }

    LOG("HS path: %s/Logs/%s", hsPathLnx.toStdString().c_str(), fileName.toStdString().c_str());
    return QString( "%1/Logs/%2" ).arg( hsPathLnx ).arg( fileName );
#elif Q_OS_WIN
    if( hsPath.isEmpty() ) {
        QString hsPathByAgent = ReadAgentAttribute( "install_dir" );

        QSettings hsKey( "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Hearthstone", QSettings::NativeFormat );
        QString hsPathByRegistry = hsKey.value( "InstallLocation" ).toString();

        if( hsPathByAgent.isEmpty() && hsPathByRegistry.isEmpty() ) {
          LOG( "Fall back to default game path" );
          hsPath = QString( getenv("PROGRAMFILES") ) + "/Hearthstone";
        } else if( !hsPathByRegistry.isEmpty() ) {
          hsPath = hsPathByRegistry;
        } else {
          hsPath = hsPathByAgent;
        }

        LOG( "Use Hearthstone location %s", qt2cstr( hsPath ) );
    }
#elif defined Q_OS_MAC
    if( hsPath.isEmpty() ) {
        hsPath = ReadAgentAttribute( "install_dir" );
        if( hsPath.isEmpty() ) {
          LOG( "Fall back to default game path" );
          hsPath = QStandardPaths::standardLocations( QStandardPaths::ApplicationsLocation ).first() + "/Hearthstone";
        }

        LOG( "Use Hearthstone location %s", qt2cstr( hsPath ) );
    }
#endif

  LOG("HS path: %s/s", hsPath.toStdString().c_str(), fileName.toStdString().c_str());
  return QString( "%1/Logs/%2" ).arg( hsPath ).arg( fileName );
}

QString Hearthstone::WindowName() const {
  QString locale = ReadAgentAttribute( "selected_locale" );
  QString windowName = "Hearthstone";

#ifdef Q_OS_MAC
  // Under mac the window name is not localized
  return windowName;
#endif

  if( locale == "zhCN" ) {
    windowName = QString::fromWCharArray( L"炉石传说" );
  } else if( locale == "zhTW" ) {
    windowName = QString::fromWCharArray( L"《爐石戰記》" );
  } else if( locale == "koKR") {
    windowName = QString::fromWCharArray( L"하스스톤" );
  }

  return windowName;
}

int Hearthstone::Width() const {
  return mCapture->Width();
}

int Hearthstone::Height() const {
  return mCapture->Height();
}

