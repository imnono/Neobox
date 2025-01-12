#include <translatedlg.h>
#include <translate.h>
#include <pluginmgr.h>
#include <pluginobject.h>
#include <neoapp.h>
#include <yjson.h>

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QTextEdit>
#include <QPushButton>
#include <QTextBlock>
#include <QVBoxLayout>
#include <QMimeData>
#include <QWidget>

extern PluginMgr* mgr;
extern GlbObject* glb;

NeoTranslateDlg::NeoTranslateDlg(YJson& settings)
    : QWidget(glb->glbGetMenu(), Qt::WindowStaysOnTopHint | Qt::Tool),
      m_Settings(settings),
      m_TextFrom(new QTextEdit(this)),
      m_TextTo(new QTextEdit(this)),
      m_BoxFrom(new QComboBox(this)),
      m_BoxTo(new QComboBox(this)),
      m_Translate(new Translate),
      m_BtnCopyFrom(new QPushButton(m_TextFrom)),
      m_BtnCopyTo(new QPushButton(m_TextTo)),
      m_BtnTransMode(new QPushButton(this)) {
  setWindowTitle("极简翻译");
  m_TextFrom->setObjectName("neoTextFrom");
  m_TextTo->setObjectName("neoTextTo");
  auto const pvLayout = new QVBoxLayout(this);
  pvLayout->addWidget(m_TextFrom);
  pvLayout->addWidget(m_TextTo);
  auto const phLayout = new QHBoxLayout;
  AddCombbox(phLayout);
  auto const pButtonGet = new QPushButton("翻译", this);
  phLayout->addWidget(pButtonGet);
  pvLayout->addLayout(phLayout);
  connect(pButtonGet, &QPushButton::clicked, this,
          std::bind(&NeoTranslateDlg::GetResultData, this));

  m_BtnCopyFrom->setText("复制");
  m_BtnCopyTo->setText("复制");
  connect(m_BtnCopyFrom, &QPushButton::clicked, this, [this]() {
    QClipboard* clip = QApplication::clipboard();
    clip->setText(m_TextFrom->toPlainText());
    m_BtnCopyFrom->setText("成功");
  });
  connect(m_BtnCopyTo, &QPushButton::clicked, this, [this]() {
    QClipboard* clip = QApplication::clipboard();
    clip->setText(m_TextTo->toPlainText());
    m_BtnCopyTo->setText("成功");
  });
  m_BtnCopyFrom->setObjectName("copyTextFrom");
  m_BtnCopyTo->setObjectName("copyTextTo");
  m_BtnCopyFrom->setCursor(Qt::PointingHandCursor);
  m_BtnCopyTo->setCursor(Qt::PointingHandCursor);

  m_TextFrom->installEventFilter(this);
  m_TextTo->installEventFilter(this);
}

NeoTranslateDlg::~NeoTranslateDlg() {
  delete m_Translate;
  delete m_TextFrom;
  delete m_TextTo;
}

void NeoTranslateDlg::showEvent(QShowEvent*) {
  auto const speedbox = ReferenceObject();

  if (m_Settings[u8"AutoTranslate"].isTrue()) {
    if (!m_TextFrom->toPlainText().isEmpty()) {
      GetResultData();
    }
  }

  if (speedbox) {
    const auto qFormRect = speedbox->frameGeometry();

    const auto size = QGuiApplication::primaryScreen()->size();
    const auto mSize = frameSize();
    int x, y;
    y = (mSize.height() + qFormRect.bottom() > size.height())
            ? qFormRect.top() - mSize.height()
            : qFormRect.bottom();
    if (((mSize.width() + qFormRect.width()) >> 1) + qFormRect.left() >
        size.width()) {
      x = size.width() - mSize.width();
    } else if ((qFormRect.left() << 1) + qFormRect.width() < mSize.width()) {
      x = 0;
    } else {
      x = qFormRect.right() - ((qFormRect.width() + mSize.width()) >> 1);
    }
    move(x, y);
  }

  m_BtnCopyFrom->move(m_TextFrom->width() - m_BtnCopyFrom->width() - 4, 4);
  m_BtnCopyTo->move(m_TextTo->width() - m_BtnCopyTo->width() - 4, 4);
  m_BtnCopyFrom->hide();
  m_BtnCopyTo->hide();

  m_TextFrom->setFocus();
  activateWindow();
}

bool NeoTranslateDlg::eventFilter(QObject* target, QEvent* event) {
  static bool bShiftDown = false, bCtrlDown = false;
  if (target == m_TextFrom || target == m_TextTo) {
    if (event->type() == QEvent::KeyPress) {
      auto const keyEvent = reinterpret_cast<QKeyEvent*>(event);
      switch (keyEvent->key()) {
        case Qt::Key_Escape:
          close();
          return true;
        case Qt::Key_Return: {
          if (bCtrlDown || m_Translate->GetSource() == Translate::Youdao) {
            GetResultData();
            return true;
          }
          break;
        }
        case Qt::Key_Alt:
          if (int i = m_BoxTo->currentIndex(); bShiftDown) {
            m_BoxTo->setCurrentIndex(i == 0 ? m_BoxTo->count() - 1 : --i);
          } else {
            m_BoxTo->setCurrentIndex(++i == m_BoxTo->count() ? 0 : i);
          }
          return true;
        case Qt::Key_Shift:
          bShiftDown = true;
          return true;
        case Qt::Key_Control:
          bCtrlDown = true;
          return true;
        case Qt::Key_M:
          if (bCtrlDown) {
            m_BtnTransMode->toggle();
            return true;
          }
        default:
          break;
      }
    } else if (event->type() == QEvent::KeyRelease) {
      auto const keyEvent = reinterpret_cast<QKeyEvent*>(event);
      if (keyEvent->key() == Qt::Key_Shift) {
        bShiftDown = false;
        return true;
      } else if (keyEvent->key() == Qt::Key_Control) {
        bCtrlDown = false;
        return true;
      }
    } else if (event->type() == QEvent::Enter) {
      if (target == m_TextFrom) {
        m_BtnCopyFrom->show();
      } else {
        m_BtnCopyTo->show();
      }
    } else if (event->type() == QEvent::Leave) {
      if (target == m_TextFrom) {
        m_BtnCopyFrom->hide();
        m_BtnCopyFrom->setText(QStringLiteral("复制"));
      } else {
        m_BtnCopyTo->hide();
        m_BtnCopyTo->setText(QStringLiteral("复制"));
      }
    }
  }
  return QWidget::eventFilter(target, event);
}

void NeoTranslateDlg::ToggleVisibility()
{
  if (isVisible()) {
    hide();
  } else {
    const auto set = m_Settings[u8"ReadClipboard"];
    if (set.isTrue()) {
      const auto *clipbord = QGuiApplication::clipboard();
      const auto *mimeData = clipbord->mimeData();
      if (mimeData->hasText()) {
        m_TextFrom->setPlainText(mimeData->text());
        return show();
      }
    }
    show();
  }
}

QWidget* NeoTranslateDlg::ReferenceObject() const
{
  return qobject_cast<QWidget*>(PluginObject::GetMainObject(u8"neospeedboxplg"));
}

void NeoTranslateDlg::GetResultData() {
  m_Translate->m_LanPair = { m_BoxFrom->currentIndex(), m_BoxTo->currentIndex() };
  auto result = m_Translate->GetResult(PluginObject::QString2Utf8(m_TextFrom->toPlainText()));
  m_TextTo->clear();
  if (m_Translate->GetSource() == Translate::Baidu)
    m_TextTo->setPlainText(PluginObject::Utf82QString(result));
  else
    m_TextTo->setHtml(PluginObject::Utf82QString(result));
}

void NeoTranslateDlg::ChangeLanguageSource(bool checked) {
  auto const newDict = checked ? Translate::Youdao: Translate::Baidu;

  m_BoxFrom->disconnect(this);
  m_BoxFrom->clear();
  m_BoxTo->disconnect(this);
  m_BoxTo->clear();

  m_Translate->SetSource(newDict);

  for (const auto& [from, tos]: m_Translate->m_LanguageCanFromTo[newDict]) {
    const auto& name = m_Translate->m_LangNameMap[from];
    m_BoxFrom->addItem(PluginObject::Utf82QString(name));
  }
  const auto& [from, tos] = m_Translate->m_LanguageCanFromTo[newDict].front();
  const auto& name = m_Translate->m_LangNameMap[from];
  m_BoxFrom->addItem(PluginObject::Utf82QString(name));
  for (auto& to : tos) {
    const auto& name = m_Translate->m_LangNameMap[to];
    m_BoxTo->addItem(PluginObject::Utf82QString(name));
  }

  connect( m_BoxFrom, &QComboBox::currentIndexChanged, this,
      std::bind(&NeoTranslateDlg::ChangeLanguageFrom, this, std::placeholders::_1));
  connect(m_BoxTo, &QComboBox::currentIndexChanged, this,
      std::bind(&NeoTranslateDlg::ChangeLanguageTo, this, std::placeholders::_1));

  m_Settings[u8"Mode"] = checked ? 1 : 0;
  mgr->SaveSettings();
}

void NeoTranslateDlg::ChangeLanguageFrom(int index) {
  m_Translate->m_LanPair.first = index;
  m_Translate->m_LanPair.second = 0;

  m_BoxTo->disconnect(this);
  m_BoxTo->clear();

  for (auto& to: m_Translate->m_LanguageCanFromTo[m_Translate->GetSource()][index].second) {
      const auto& name = m_Translate->m_LangNameMap[to];
      m_BoxTo->addItem(PluginObject::Utf82QString(name));
  }

  connect(m_BoxTo, &QComboBox::currentIndexChanged, this,
      std::bind(&NeoTranslateDlg::ChangeLanguageTo, this, std::placeholders::_1));
}

void NeoTranslateDlg::ChangeLanguageTo(int index)
{
  m_Translate->m_LanPair.second = index;
}

void NeoTranslateDlg::AddCombbox(QHBoxLayout* layout) {
  layout->addWidget(m_BoxFrom);
  QLabel* lable = new QLabel(this);
  lable->setText(QStringLiteral("→"));
  layout->addWidget(lable);
  layout->addWidget(m_BoxTo);

  m_BtnTransMode->setObjectName(QStringLiteral("btnTransMode"));
  m_BtnTransMode->setCheckable(true);
  m_BtnTransMode->setText("查词");
  layout->addWidget(m_BtnTransMode);
  auto const dict = (Translate::Source)m_Settings[u8"Mode"].getValueInt();
  m_Translate->SetSource(dict);
  m_BtnTransMode->setChecked(dict == Translate::Youdao);
  ChangeLanguageSource(dict);

  connect(m_BtnTransMode, &QPushButton::toggled, this, std::bind(&NeoTranslateDlg::ChangeLanguageSource, this, std::placeholders::_1));
  connect( m_BoxFrom, &QComboBox::currentIndexChanged, this,
      std::bind(&NeoTranslateDlg::ChangeLanguageFrom, this, std::placeholders::_1));
  connect(m_BoxTo, &QComboBox::currentIndexChanged, this,
      std::bind(&NeoTranslateDlg::ChangeLanguageTo, this, std::placeholders::_1));

}
