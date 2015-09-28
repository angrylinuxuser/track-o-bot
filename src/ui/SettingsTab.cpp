#include "SettingsTab.h"
#include "ui_SettingsWidget.h"

#include "../Settings.h"

SettingsTab::SettingsTab( QWidget *parent )
  : QWidget( parent ), mUI( new Ui::SettingsWidget )
{
  mUI->setupUi( this );
  connect( mUI->startAtLogin, SIGNAL( clicked(bool) ), this, SLOT( UpdateAutostart() ) );
#if defined Q_OS_LINUX
  mUI->checkForUpdatesNowButton->hide();
  mUI->checkForUpdates->setDisabled(true);
#else
  connect( mUI->checkForUpdatesNowButton, SIGNAL( clicked() ), this, SLOT( CheckForUpdatesNow() ) );
  connect( mUI->checkForUpdates, SIGNAL( clicked(bool) ), this, SLOT( UpdateAutoUpdateCheck() ) );
#endif
  LoadSettings();
}

SettingsTab::~SettingsTab() {
  delete mUI;
}

void SettingsTab::CheckForUpdatesNow() {
  Settings::Instance()->CheckForUpdates();
}

void SettingsTab::UpdateAutostart() {
  Settings::Instance()->SetAutostart( mUI->startAtLogin->isChecked() );
}

void SettingsTab::UpdateAutoUpdateCheck() {
  Settings::Instance()->SetAutoUpdateCheck( mUI->checkForUpdates->isChecked() );
}

void SettingsTab::LoadSettings() {
  Settings *settings = Settings::Instance();

  mUI->startAtLogin->setChecked( settings->Autostart() );
  mUI->checkForUpdates->setChecked( settings->AutoUpdateCheck() );
}
