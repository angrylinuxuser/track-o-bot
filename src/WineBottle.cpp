#include "WineBottle.h"
#include <QDebug>

#define SYSTEM_REG "system.reg"
#define USER_REG "user.reg"
#define USERDEF_REG "userdef.reg"
#define DOSDEVICES "dosdevices"
#define DRIVE_C "drive_c"

WineBottle::WineBottle( const QString& prefix )
	: mWinePrefix( prefix ),
		mSystemReg( nullptr ),
		mUserReg( nullptr ),
		mUserdefReg( nullptr ),
		mRegFormat( QSettings::registerFormat( "reg", ReadWineRegFile, nullptr ) ) {
	SetupDosDevices();
}

WineBottle::~WineBottle() {
	if ( mSystemReg )
		delete mSystemReg;
	if ( mUserReg )
		delete mUserReg;
	if ( mUserdefReg )
		delete mUserdefReg;
}

const QString& WineBottle::Path() {
	return mWinePrefix;
}

QString WineBottle::DosDevicePath(const QChar& drive) {
	return mDosDevices[ drive.toLower() ];
}

bool WineBottle::IsValid() {
	QStringList matchList;
	matchList << DOSDEVICES << DRIVE_C
						<< SYSTEM_REG << USER_REG << USERDEF_REG;

	int count = 0;
	foreach( const QFileInfo& info, QDir( mWinePrefix ).entryInfoList()  ) {
		if ( matchList.contains( info.fileName() ) )
				 ++count;
	}

	if ( count == matchList.size() )
		return true;

	return false;
}

QVariant WineBottle::ReadRegistryValue( const QString& key ) {

	if ( mSystemReg == nullptr )
		mSystemReg = new QSettings( mWinePrefix + "/" + SYSTEM_REG , mRegFormat );
	if ( mUserReg == nullptr )
		mUserReg = new QSettings( mWinePrefix + "/" + USER_REG , mRegFormat );
	if ( mUserdefReg == nullptr )
		mUserdefReg = new QSettings( mWinePrefix + "/" + USERDEF_REG, mRegFormat );

	if ( mSystemReg->value( key ).isValid() )
		return mSystemReg->value( key );
	else if ( mUserReg->value( key ).isValid() )
		return mUserReg->value( key );
	else if ( mUserdefReg->value( key ).isValid() )
		return mUserdefReg->value( key );
	else
		return QVariant();
}

QString WineBottle::ToSystemPath(const QString& windowsStylePath ) {

	QString path = windowsStylePath;
	QString drivePath = DosDevicePath( windowsStylePath.at( 0 ) );

	path.replace( "\\\\", QDir::separator() );
	path[ 0 ].toLower();

	if ( !drivePath.endsWith( QDir::separator() ) )
		drivePath.append( QDir::separator() );

	return path.replace( 0, 3, drivePath );
}

// gather all "drives" for this wine prefix
void WineBottle::SetupDosDevices() {
	QDir prefix( mWinePrefix );
	if ( prefix.cd( DOSDEVICES ) ) {
		/**
		 *  dosdevices dir should contain (directories) symlinks to "drives"
		 *  i.e. 'c:', 'd:'....
		 **/
		foreach ( const QFileInfo& dosdevice, prefix.entryInfoList( QDir::Dirs | QDir::NoDotAndDotDot	 ) ) {
			if ( ( dosdevice.baseName().size() == 2 ) &&
					 ( dosdevice.baseName().at(0).isLetter() ) &&
					 ( dosdevice.baseName().at(1) == ':' ) ) {
				mDosDevices.insert( dosdevice.baseName().at(0), dosdevice.symLinkTarget() );
			}
		}
	}
}

// lets blame this on my poor programming skills
bool WineBottle::ReadWineRegFile(QIODevice& device, QSettings::SettingsMap& map) {

	if ( !device.isOpen() )
		return false;

	device.setTextModeEnabled( true );
	QTextStream inputStream( &device );

	// first line should be: WINE REGISTRY Version 2
	if ( inputStream.readLine() != "WINE REGISTRY Version 2" ) {
		return false;
	}
	// determine what prefix needs to be prepend

	QString line = inputStream.readLine();
	QString defaultPrefix, prefix;

	if ( line == ";; All keys relative to \\\\Machine" ) {
		// system.reg
		// ;; All keys relative to \\Machine
		defaultPrefix = "HKEY_LOCAL_MACHINE/";
	}
	else if ( line == ";; All keys relative to \\\\User\\\\.Default") {
		// userdef.reg
		defaultPrefix = "HKEY_USERS/.Default/";
	}
	else if ( line.left( 32 ) == ";; All keys relative to \\\\User\\\\") {
		// has to be user.reg since we cought userdef earlier
		// ;; All keys relative to \\User\\x-x-x-x-x-x-x-xxxx
		// resides in two places:
		// HKEY_CURRENT_USER/ and HKEY_USERS/User/x-x-x-x-x-x-x-xxxx
		// howerever we need only current user one
		defaultPrefix = "HKEY_CURRENT_USER/";
	}
	else {
		qDebug() << "Error: Could not determine prefix!";
		return false;
	}

	// now we fill SettingsMap
	QString key, valueString;
	QStringList explode;
	QVariant value;

	while ( !inputStream.atEnd() ) {
		line = inputStream.readLine();

		if ( line.startsWith( '#' ) ) {} 				/* skip comment */
		/* empty line means that next section is starting so prefix has to be cleared */
		else if ( line.startsWith( '\n' ) ) {
			prefix.clear();
		}
		else if ( line.startsWith( '[' ) ) { 		/* found new section */
			prefix = ( line.mid( 1, line.indexOf( "]" ) - 1 ) ).
							 replace( "\\\\", QDir::separator() );
			prefix.prepend( defaultPrefix );
		}
		/* found key */
		//else if ( line.startsWith( '\' ) || line.startsWith( '@' ) )
		else if ( line.startsWith( '"' ) ) {
			explode = line.split( '=' );

			key =  explode[ 0 ].replace( "\"" , "" );
			// cannot replace " cuz we dont know the type
			valueString = explode[ 1 ];

			// REG_SZ
			if ( valueString.startsWith( '"' ) && valueString.endsWith( '"' )) {
				value.setValue( valueString.mid( 1, valueString.length() - 2 ));
			}
			// REG_DWORD
			else if ( valueString.startsWith( "dword:" )) {
				unsigned long dword = valueString.mid( 6 ).toULong();
				value.setValue( dword );
			}
			// REG_BINARY do i even need this?
			// TODO: finish this someday
//			else if ( valueString.startsWith( "hex:")) {
//			}
//			// REG_MULTI_SZ */
//			else if ( valueString.startsWith( "str(" ) ) {
//			}
//
			/* insert key and value to map */
			map [ prefix + "/" + key ] = value;
			key.clear();
			value.clear();
		} else {
			// unhandeled stuff we dont need
		}
	}
	return true;
}
