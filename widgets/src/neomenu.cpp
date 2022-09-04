#include <neomenu.h>
#include <sysapi.h>
#include <varbox.h>
#include <wallpaper.h>
#include <yjson.h>
#include <screenfetch.h>
#include <translate.h>

#include <map>

#include <QActionGroup>
#include <QApplication>
#include <QDesktopServices>
#include <QFile>
#include <QFileDialog>
#include <QInputDialog>
#include <QStandardPaths>
#include <QUrl>
#include <QMessageBox>
#include <QEventLoop>

#ifdef _WIN32
#include <Windows.h>
#elif def __linux__
#else
#endif

extern QString QImage2Text(const QImage& qImage);

NeoMenu::NeoMenu(QWidget* parent) : QMenu(parent),
  m_Wallpaper(new Wallpaper(&VarBox::GetSettings(u8"Wallpaper"), &VarBox::WriteSettings))
{
  setAttribute(Qt::WA_TranslucentBackground, true);
  InitFunctionMap();
  QFile fJson(QStringLiteral(":/jsons/menucontent.json"));
  if (!fJson.open(QIODevice::ReadOnly)) {
    qApp->quit();
    return;
  }
  QByteArray array = fJson.readAll();
  GetMenuContent(this, YJson(array.begin(), array.end()));
  fJson.close();

  LoadWallpaperExmenu();
}

NeoMenu::~NeoMenu() {
  delete m_Wallpaper;
}

void NeoMenu::InitFunctionMap() {
  m_FuncNormalMap = {
      {u8"SystemShutdown", std::bind(ShellExecuteW, nullptr, L"open",
                                     L"shutdown", L"-s -t 0", nullptr, 0)},
      {u8"SystemRestart", std::bind(ShellExecuteW, nullptr, L"open",
                                    L"shutdown", L"-r -t 0", nullptr, 0)},
      {u8"AppQuit", &QApplication::quit},
      {u8"AppOpenExeDir",
       std::bind(QDesktopServices::openUrl,
                 QUrl::fromLocalFile(qApp->applicationDirPath()))},
      {u8"AppOpenWallpaperDir",
       [this]() {
         QDesktopServices::openUrl(QUrl::fromLocalFile(
             QString::fromStdWString(m_Wallpaper->GetImageDir().wstring())));
       }},
      {u8"AppOpenConfigDir",
       std::bind(QDesktopServices::openUrl,
                 QUrl::fromLocalFile(QDir::currentPath()))},
      {u8"ToolOcrGetScreen",
       [this]() {
         QImage image;
         ScreenFetch* box = new ScreenFetch(image);
         QEventLoop loop;
         connect(box, &ScreenFetch::destroyed, &loop, &QEventLoop::quit);
         box->showFullScreen();
         loop.exec();
         Translate translate(this);
         translate.Show(QImage2Text(image));
       }},
      {u8"WallpaperPrev", std::bind(&Wallpaper::SetSlot, m_Wallpaper, -1)},
      {u8"WallpaperNext", std::bind(&Wallpaper::SetSlot, m_Wallpaper, 1)},
      {u8"WallpaperDislike", std::bind(&Wallpaper::SetSlot, m_Wallpaper, 0)},
      {u8"WallpaperUndoDislike",
       std::bind(&Wallpaper::UndoDelete, m_Wallpaper)},
      {u8"WallpaperCollect", std::bind(&Wallpaper::SetFavorite, m_Wallpaper)},
      {u8"WallpaperUndoCollect",
       std::bind(&Wallpaper::UnSetFavorite, m_Wallpaper)},
      {u8"WallpaperDir",
       [this]() {
         QString current =
             QString::fromStdWString(m_Wallpaper->GetCurIamge().wstring());
         if (!ChooseFolder("选择壁纸文件夹", current))
           return;
         m_Wallpaper->SetCurDir(current.toStdWString());
       }},
      {u8"WallpaperTimeInterval",
       [this]() {
         int iNewTime = QInputDialog::getInt(this, "输入时间间隔",
                                             "时间间隔（分钟）：", m_Wallpaper->GetTimeInterval(), 5);
         m_Wallpaper->SetTimeInterval(iNewTime);
       }},
      {u8"WallpaperClean", std::bind(&Wallpaper::ClearJunk, m_Wallpaper)},
      {u8"AppWbsite", std::bind(QDesktopServices::openUrl,
                                QUrl("https://www.github.com/yjmthu/Neobox"))}};

  m_FuncCheckMap = {
      {u8"AppAutoSatrt",
       {[](bool checked) {
          wchar_t pPath[] =
              L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
          wchar_t pAppName[] = L"Neobox";
          std::wstring wsThisPath = GetExeFullPath();
          std::wstring wsThatPath =
              RegReadString(HKEY_CURRENT_USER, pPath, pAppName);
          if (wsThatPath != wsThisPath) {
            if (checked) {
              RegWriteString(HKEY_CURRENT_USER, pPath, pAppName, wsThisPath);
            }
          } else {
            if (!checked) {
              RegRemoveValue(HKEY_CURRENT_USER, pPath, pAppName);
            }
          }
        },
        []() -> bool {
          wchar_t pPath[] =
              L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
          return GetExeFullPath() ==
                 RegReadString(HKEY_CURRENT_USER, pPath, L"Neobox");
        }}},
      {u8"OtherSetDesktopRightMenu",
       {[](bool checked){
          constexpr auto prefix = L"Software\\Classes\\{}\\shell";
          const std::initializer_list<std::pair<std::wstring, wchar_t>> lst = {
            {L"*", L'1'}, {L"Directory", L'V'}, {L"Directory\\Background", L'1'}};
          constexpr auto command = L"mshta vbscript:clipboarddata.setdata(\"text\",\"%{}\")(close)";
          if (checked) {
            for (const auto& [param1, param2]: lst) {
              HKEY hKey = nullptr;
              std::wstring wsSubKey = std::format(prefix, param1) + L"\\Neobox";
              std::wstring cmdstr = std::format(command, param2);
              std::wstring wsIconFileName = std::filesystem::absolute("icons/copy.ico").wstring();
              if (RegCreateKeyW(HKEY_CURRENT_USER, wsSubKey.c_str(), &hKey) != ERROR_SUCCESS) {
                return;
              }
              RegSetValueW(hKey, L"command", REG_SZ, cmdstr.c_str(), 0);
              RegSetValueW(hKey, nullptr, REG_SZ, L"复制路径", 0);
              RegSetValueExW(hKey, L"Icon", 0, REG_SZ, reinterpret_cast<const BYTE*>(wsIconFileName.data()), (DWORD)wsIconFileName.size()*sizeof(wchar_t));
              RegCloseKey(hKey);
            }
          } else {
            for (const auto& [param1, param2]: lst) {
              HKEY hKey = nullptr;
              std::wstring wsSubKey = std::format(prefix, param1);
              if (RegOpenKeyExW(HKEY_CURRENT_USER, wsSubKey.c_str(), 0, KEY_WRITE, &hKey) != ERROR_SUCCESS) {
                continue;
              }
              RegDeleteTreeW(hKey, L"Neobox");
              RegCloseKey(hKey);
            }
          }
        },
        []()->bool{
          constexpr auto prefix = L"Software\\Classes\\{}\\shell";
          const auto lst = { L"*", L"Directory", L"Directory\\Background" };
          for (const auto& i: lst) {
            std::wstring wsSubKey = std::format(prefix, i);
            HKEY hKey = nullptr;
            if (RegOpenKeyExW(HKEY_CURRENT_USER, wsSubKey.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
              QMessageBox::information(nullptr, "1", QString::fromStdWString(wsSubKey));
              return false;
            }
            RegCloseKey(hKey);
          }
          return true;}}},
      {u8"WallpaperAutoChange",
       {std::bind(&Wallpaper::SetAutoChange, m_Wallpaper, std::placeholders::_1),
        std::bind(&Wallpaper::GetAutoChange, m_Wallpaper)}},
      {u8"WallpaperInitChange",
       {std::bind(&Wallpaper::SetFirstChange, m_Wallpaper, std::placeholders::_1),
        std::bind(&Wallpaper::GetFirstChange, m_Wallpaper)}}};

  m_FuncItemCheckMap = {
      {u8"WallpaperType",
       {[this](int type) {
          m_Wallpaper->SetImageType(type);
          LoadWallpaperExmenu();
        },
        std::bind(&Wallpaper::GetImageType, m_Wallpaper)}}};
}

bool NeoMenu::ChooseFolder(QString title, QString& current) {
  current = QFileDialog::getExistingDirectory(this, title, current);
  return QDir().exists(current);
}

void NeoMenu::GetMenuContent(QMenu* parent, const YJson& data) {
  for (const auto& [i, j] : data.getObject()) {
    QAction* action = parent->addAction(QString::fromUtf8(i.data(), i.size()));
    const std::u8string type = j[u8"type"].second.getValueString();
    if (type == u8"Normal") {
      const auto& function =
          m_FuncNormalMap[j[u8"function"].second.getValueString()];
      connect(action, &QAction::triggered, this, function);
    } else if (type == u8"Group") {
      QMenu* menu = new QMenu(parent);
      action->setMenu(menu);
      GetMenuContent(menu, j[u8"children"].second);
    } else if (type == u8"Checkable") {
      const auto& [m, n] =
          m_FuncCheckMap[j[u8"function"].second.getValueString()];
      action->setCheckable(true);
      action->setChecked(n());
      connect(action, &QAction::triggered, this, m);
    } else if (type == u8"ExclusiveGroup") {
      const auto& function =
          m_FuncItemCheckMap[j[u8"function"].second.getValueString()].second;
      QMenu* menu = new QMenu(parent);
      action->setMenu(menu);
      const auto& children = j[u8"children"].second;
      GetMenuContent(menu, children);

      const auto& range = j[u8"range"].second.getArray();
      const int first = range.front().getValueInt();
      int last = range.back().getValueInt();
      if (last == -1)
        last = static_cast<int>(children.getObject().size());
      int index = 0, check = function();
      QActionGroup* group = new QActionGroup(menu);
      group->setExclusive(true);

      for (auto& k : menu->actions()) {
        if (index < first) {
          continue;
        } else if (index >= last) {
          break;
        } else {
          group->addAction(k);
          k->setChecked(index == check);
          ++index;
        }
      }
    } else if (type == u8"GroupItem") {
      const auto& function =
          m_FuncItemCheckMap[j[u8"function"].second.getValueString()].first;
      action->setCheckable(true);
      connect(action, &QAction::triggered, this,
              std::bind(function, j[u8"index"].second.getValueInt()));
    } else if (type == u8"VarGroup") {
      QMenu *menu = new QMenu(parent);
      action->setMenu(menu);
      m_ExMenus[j[u8"name"].second.getValueString()] = menu;
    }
  }
}

void NeoMenu::LoadWallpaperExmenu()
{
  QMenu * pMainMenu = m_ExMenus[u8"Wallpaper"];
  pMainMenu->clear();
  YJson* jsExInfo = m_Wallpaper->m_Wallpaper->GetJson();
  switch (m_Wallpaper->GetImageType()) {
   case WallBase::WALLHAVEN: {
    QActionGroup* group = new QActionGroup(pMainMenu);
    for (auto& [u8TypeName, data]: jsExInfo->find(u8"WallhavenApi")->second.getObject()) {
      QAction* action = pMainMenu->addAction(
          QString::fromUtf8(u8TypeName.data(), u8TypeName.size()));
      action->setCheckable(true);
      action->setChecked(u8TypeName == jsExInfo->find(u8"WallhavenCurrent")->second.getValueString());
      group->addAction(action);
      QMenu* pSonMenu = new QMenu(pMainMenu);
      action->setMenu(pSonMenu);
      QAction* temp = nullptr;
      temp = pSonMenu->addAction(action->isChecked()?"刷新此项":"启用此项");
      connect(temp, &QAction::triggered, this, [this, action](){
        action->setChecked(true);
        QByteArray&& array = action->text().toUtf8();
        auto js = m_Wallpaper->m_Wallpaper->GetJson();
        js->find(u8"WallhavenCurrent")->second.setText(array.begin(), array.end());
        m_Wallpaper->m_Wallpaper->SetJson();
      });
      temp = pSonMenu->addAction("参数设置");
      temp = pSonMenu->addAction("存储路径");
    }
    pMainMenu->addSeparator();
    pMainMenu->addAction("添加更多");
    pMainMenu->addAction("关于壁纸");
    break;
   } case WallBase::BINGAPI:
   case WallBase::NATIVE:
   default:
    break;
  }
  pMainMenu->addAction("打开位置");
}

