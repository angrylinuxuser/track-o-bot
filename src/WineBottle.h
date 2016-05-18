#ifndef WINEBOTTLE_H
#define WINEBOTTLE_H

#include <QString>
#include <QSettings>
#include <QDir>
#include <QList>
#include <QMap>

class WineBottle {
public:
  /**
   * @brief Wine prefix
   * @param prefix is a path to the directory containing wine's prefix
   */
  WineBottle( const QString& Path );
  /**
   * @brief destructor
   */
  ~WineBottle();
  /**
   * @brief Returns path to Wine prefix
   * @retun wine prefix
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
   *    and the path is d:\\mnt\\drive\Hearthstone
   *    then we get /mnt/drive/Hearthstone
   * @param windows style path
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
private:
  /**
   * @brief Scans prefix to set up m_dosDevices
   */
  void SetupDosDevices();
  /**
   * @brief Custom format readder ( *.reg ) for QSettings
   */
  static bool ReadWineRegFile( QIODevice& device, QSettings::SettingsMap& map );
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

	const QString mWinePrefix;                /**< path to the prefix */
	QMap< QChar, QString > mDosDevices; /**< "dosDrives" mapped to their symlinked locations */
	QSettings* mSystemReg;              /**< HKEY_LOCAL_MACHINE keys */
	QSettings* mUserReg;                /**<  HKEY_USERS/.Default keys */
	QSettings* mUserdefReg;             /**< HKEY_CURRENT_USER keys */
	const QSettings::Format mRegFormat; /**< custom registry format */
};

#endif // WINEBOTTLE_H
