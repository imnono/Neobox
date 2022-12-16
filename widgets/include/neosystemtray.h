#ifndef NEOSYSTEMTRAY_H
#define NEOSYSTEMTRAY_H

#include <QSystemTrayIcon>
#include <pluginobject.h>

class NeoSystemTray: public QSystemTrayIcon
{
public:
  explicit NeoSystemTray();
  virtual ~NeoSystemTray();
  std::set<const PluginObject::FollowerFunction*> m_Followers;
private:
  void InitDirs();
  void InitConnect();
};

#endif // NEOSYSTEMTRAY_H