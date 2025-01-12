#ifndef TRANSLATEDLG_H
#define TRANSLATEDLG_H

#include <QWidget>

class NeoTranslateDlg : public QWidget {
 protected:
  void showEvent(QShowEvent*) override;
  bool eventFilter(QObject*, QEvent*) override;

 public:
  explicit NeoTranslateDlg(class YJson& setings);
  ~NeoTranslateDlg();
  void ToggleVisibility();

 private:
  void GetResultData();
  class YJson& m_Settings;
  class QWidget* ReferenceObject() const;
  class QTextEdit *m_TextFrom, *m_TextTo;
  class QComboBox *m_BoxFrom, *m_BoxTo;
  class Translate* m_Translate;
  class QPushButton *m_BtnCopyFrom, *m_BtnCopyTo;
  class QPushButton* m_BtnTransMode;
  void AddCombbox(class QHBoxLayout* layout);
  void ChangeLanguageSource(bool checked);
  void ChangeLanguageFrom(int index);
  void ChangeLanguageTo(int index);
};

#endif // TRANSLATEDLG_H
