#ifndef WINEBOTTLE_H
#define WINEBOTTLE_H

#include <QString>
#include <QSettings>
#include <QDir>
#include <QList>
#include <QMap>

#define REG_NONE		0	/* no type */
#define REG_SZ			1	/* string type (ASCII) */
#define REG_EXPAND_SZ		2	/* string, includes %ENVVAR% (expanded by caller) (ASCII) */
#define REG_BINARY		3	/* binary format, callerspecific */
/* YES, REG_DWORD == REG_DWORD_LITTLE_ENDIAN */
#define REG_DWORD		4	/* DWORD in little endian format */
#define REG_DWORD_LITTLE_ENDIAN	4	/* DWORD in little endian format */
#define REG_DWORD_BIG_ENDIAN	5	/* DWORD in big endian format  */
#define REG_LINK		6	/* symbolic link (UNICODE) */
#define REG_MULTI_SZ		7	/* multiple strings, delimited by \0, terminated by \0\0 (ASCII) */
#define REG_RESOURCE_LIST	8	/* resource list? huh? */
#define REG_FULL_RESOURCE_DESCRIPTOR	9	/* full resource descriptor? huh? */
#define REG_RESOURCE_REQUIREMENTS_LIST	10
#define REG_QWORD		11	/* QWORD in little endian format */
#define REG_QWORD_LITTLE_ENDIAN	11	/* QWORD in little endian format */

#define MAX_REG_FILES 3
#define SYSTEM_REG "system.reg"
#define SYSTEM_REG_ID 0
#define USER_REG "user.reg"
#define USER_REG_ID 1
#define USERDEF_REG "userdef.reg"
#define USERDEF_REG_ID 2

#define REG_KEY_LOCAL_APPDATA "HKEY_USERS/.Default/Software/Microsoft/Windows/CurrentVersion/Explorer/Shell Folders/Local AppData"
#define REG_KEY_HS_INSTALL_LOCATION "HKEY_LOCAL_MACHINE/Software/Microsoft/Windows/CurrentVersion/Uninstall/Hearthstone/InstallLocation"
#define REG_KEY_APPDATA "HKEY_USERS/.Default/Software/Microsoft/Windows/CurrentVersion/Explorer/Shell Folders/AppData"

class QTextStream;

class WineBottle {
  DEFINE_SINGLETON( WineBottle )
public:
  /**
   * @brief Sets wine prefix path
   * @param QString
   */
  void SetPath( const QString& path );
  /**
   * @brief Returns path to Wine prefix
   * @return wine prefix
   */
  const QString& Path() const;
  /**
   * @brief Translates windows style path to system's path
   *
   * For examlple:
   *    assuming we have 2 dosdevices, c: and d:
   * 		c: -> {$wineprefix}/drive_c
   *    d: -> /
   *
   *    and the path is d:\\mnt\\drive\\Hearthstone
   *    then we get /mnt/drive/Hearthstone
   * @param windows style path
   * @reurns translated path or empty string if error
   */
  QString ToSystemPath( const QString& windowsStylePath ) const;
  /**
   * @brief is WineBottle a valid wine prefix
   * @return true if yes, false if no
   */
  bool IsValid() const;
  /**
   * @brief Reads values stored in wine's registry
   * @param key
   * @return value for a given key or QVariant(invalid) if none is found/error
   */
  QVariant ReadRegistryValue( const QString& key );
  /**
   * @brief Resets
   */
  void ResetRegFile(int id);
private:
  /**
   * @brief Scans prefix to set up mDosDevices
   */
  void SetupDosDevices();
  /**
   * @brief Custom format readder ( *.reg ) for QSettings
   */
  static bool ReadWineRegFile( QIODevice& device, QSettings::SettingsMap& map );
  /**
   * @brief WriteWineRegFile
   * @param device
   * @param map
   * @return 0
   *
   * Dummy method.
   */
  static bool WriteWineRegFile( QIODevice& device, const QSettings::SettingsMap& map );
  /**
   * @brief Returns absolute path to the Wine's "drive"
   *
   * "Drives" are stored in {$wineprefix}/dosdevices/ as symlinks
   * to directories
   *
   * @param drive
   * @return absolute path to the drive
   */
  QString DosDevicePath( const QChar& drive ) const;
  QString mWinePrefix;                	/**< path to the prefix */
  QMap< QChar, QString > mDosDevices; 	/**< "dosDrives" mapped to their symlinked locations */
  QSettings* mRegFiles[ MAX_REG_FILES ];/**< array of settings from system.reg, user.reg, userdef.reg */
  const QSettings::Format mRegFormat; 	/**< custom registry format */
  bool mIsValid;                      	/**< is wine prefix a valid one */
};

#define Wine WineBottle::Instance()

#endif // WINEBOTTLE_H
