#include <QtWidgets/QStyleFactory>

#include "cle_main.h"

/* these must last for the lifetime of the QApplication */
static int app_argc = 1;
static char app_name[] = "Live Editor";
static char *app_argv[] = { app_name, nullptr };

extern "C"
{
void cle_init()
{
  m_Application = new QApplication(app_argc, app_argv);

  QPalette darkPalette;
  darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
  darkPalette.setColor(QPalette::WindowText, Qt::white);
  darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
  darkPalette.setColor(QPalette::AlternateBase, QColor(30, 30, 30));
  darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
  darkPalette.setColor(QPalette::ToolTipText, Qt::white);
  darkPalette.setColor(QPalette::Text, Qt::white);
  darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
  darkPalette.setColor(QPalette::ButtonText, Qt::white);
  darkPalette.setColor(QPalette::BrightText, Qt::red);
  darkPalette.setColor(QPalette::Link, QColor(26, 183, 234));
  darkPalette.setColor(QPalette::Highlight, QColor(26, 183, 234));
  darkPalette.setColor(QPalette::HighlightedText, Qt::black);

  QStyle *fusionStyle = QStyleFactory::create("Fusion");
  const QString editorSS = "QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }";

  auto applyEditorTheme = [&](QWidget *w) {
    w->setStyle(fusionStyle);
    w->setPalette(darkPalette);
    w->setStyleSheet(editorSS);
    for (QWidget *child : w->findChildren<QWidget *>())
    {
      child->setStyle(fusionStyle);
      child->setPalette(darkPalette);
    }
  };

  m_MemoryInspector = new CleMemoryInspector();
  m_MemoryInspector->show();
  applyEditorTheme(m_MemoryInspector);

  m_MemoryNotes = new CleMemoryNotes(nullptr);
  m_MemoryNotes->show();
  applyEditorTheme(m_MemoryNotes);

  m_ScriptEditor = new CleScriptEditorBlock();
  m_ScriptEditor->show();
  applyEditorTheme(m_ScriptEditor);
}

void cle_run()
{
  m_MemoryNotes->update();
}
}
