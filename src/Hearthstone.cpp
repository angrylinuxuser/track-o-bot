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
#include "WineBottle.h"
#include "Settings.h"
#endif

DEFINE_SINGLETON_SCOPE( Hearthstone );

Hearthstone::Hearthstone()
 : mCapture( NULL ), mGameRunning( false )
{
#ifdef Q_OS_MAC
    LOG("OS X host");
  mCapture = new OSXWindowCapture();
#elif defined Q_OS_WIN
    LOG("Windows host");
  mCapture = new WinWindowCapture();
#elif defined Q_OS_LINUX
 LOG("GNU/Linux host");
 mCapture = new LinuxWindowCapture();
#endif

  // On OS X, WindowFound is quite CPU intensive
  // Starting time for HS is also long
  // So just check only once in a while
  mTimer = new QTimer( this );
  connect( mTimer, &QTimer::timeout, this, &Hearthstone::Update );
#ifdef Q_OS_MAC
  mTimer->start( 5000 );
#else
  mTimer->start( 250 );
#endif
}

Hearthstone::~Hearthstone() {
  if( mCapture != NULL )
    delete mCapture;
}

void Hearthstone::Update() {
	bool isRunning = mCapture->WindowFound();

	if( isRunning ) {
	#ifdef Q_OS_LINUX
		emit FocusChanged( mCapture->Focus() );
	#endif
		static int lastLeft = 0, lastTop = 0, lastWidth = 0, lastHeight = 0;
		if( lastLeft != mCapture->Left() || lastTop != mCapture->Top() ||
				lastWidth != mCapture->Width() || lastHeight != mCapture->Height() )
		{
			lastLeft = mCapture->Left(),
			lastTop = mCapture->Top(),
			lastWidth = mCapture->Width(),
			lastHeight = mCapture->Height();

      DBG( "HS window changed %d %d %d %d", lastLeft, lastTop, lastWidth, lastHeight );
      emit GameWindowChanged( lastLeft, lastTop, lastWidth, lastHeight );
    }
  }

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
    DBG( "Couldn't open %s (%d)", qt2cstr( path ), file.error() );
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
  QString configPath;

  WineBottle bottle( Settings::Instance()->WinePrefixPath() );
  if( bottle.IsValid() ) {
    QString path =
        bottle.ReadRegistryValue( "HKEY_USERS/.Default/Software/Microsoft/Windows/CurrentVersion/Explorer/Shell Folders/Local AppData" ).toString();
    QString localAppData = bottle.ToSystemPath( path );
    configPath = localAppData + "/Blizzard/Hearthstone/log.config";
  } else {
    LOG( "Invalid wine prefix. Cannot determine path for log.config" );
  }
#endif
  return configPath;
}

QString Hearthstone::DetectHearthstonePath() const {
  static QString hsPath;

#ifdef Q_OS_LINUX
    QString winePrefix = DetectWinePrefixPath();
    // cant find wine prefix so dont look for hs path
    if ( winePrefix.isEmpty() )
      return hsPath;

		if ( hsPath.isEmpty() ) {
			WineBottle bottle( winePrefix );
			// path is windows style ie c:\\\Program Files\\..
			QString path =
                bottle.ReadRegistryValue( "HKEY_LOCAL_MACHINE/Software/Microsoft/Windows/CurrentVersion/Uninstall/Hearthstone/InstallLocation" ).toString();

			if ( !path.isEmpty() ) {
				// need to translate path to *nix style path
				path = bottle.ToSystemPath( path );

				if ( QDir( path ).exists() )
					hsPath = path;
				else
					LOG( "Game folder not found (%s). You should set the path manually in the settings!", qt2cstr( path ) );
			}
		}

#elif Q_OS_WIN
    if( hsPath.isEmpty() ) {
        QString hsPathByAgent = ReadAgentAttribute( "install_dir" );
        QSettings hsKey( "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Hearthstone", QSettings::NativeFormat );
        QString hsPathByRegistry = hsKey.value( "InstallLocation" ).toString();

        if( hsPathByAgent.isEmpty() && hsPathByRegistry.isEmpty() ) {
            LOG( "Game folder not found. Fall back to default game path for now. You should set the path manually in the settings!" );
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
      LOG( "Fall back to default game path. You should set the path manually in the settings!" );
      hsPath = QStandardPaths::standardLocations( QStandardPaths::ApplicationsLocation ).first() + "/Hearthstone";
    }
#endif
  return hsPath;
}

int Hearthstone::Width() const {
  return mCapture->Width();
}

int Hearthstone::Height() const {
  return mCapture->Height();
}

QString Hearthstone::DetectWinePrefixPath() const
{
  static QString winePrefix;

  if ( winePrefix.isEmpty() ) {
    // default prefix
    WineBottle bottle( QDir::homePath() + "/.wine" );
    if ( bottle.IsValid() )
      winePrefix =  bottle.Path();
    else
      ERR( "Could not determine Wine prefix directory" );
  }
  return winePrefix;
}
