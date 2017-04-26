#ifndef WINEBOTTLE_H
#define WINEBOTTLE_H

#include <QString>
#include <QSettings>
#include <QMap>
#include <QPointer>

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
   * @brief is WineBottle a valid wine prefix
   * @return true if yes, false if no
   */
  bool IsValid() const;
  /**
   * @brief returns Wine path to Local AppData directory from registry
   * @return QString
   */
  QString LocalAppData() const;
  QString HearthstoneInstallLocation() const;
  QString AppData() const;
private:
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
   * @brief Reads values stored in wine's registry
   * @param key
   * @return value for a given key or QVariant(invalid) if none is found/error
   */
  QVariant ReadRegistryValue( const QString& key ) const;
  /**
   * @brief Scans prefix to set up mDosDevices
   */
  void SetupDosDevices();

  void SetupRegFiles();
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
  QVector< QPointer< QSettings >> mRegFiles;        /**< vector of settings from system.reg, user.reg, userdef.reg */
  const QSettings::Format mRegFormat; 	/**< custom registry format */
  bool mIsValid;                      	/**< is wine prefix a valid one */
};

#define WinePrefix WineBottle::Instance()

#endif // WINEBOTTLE_H
