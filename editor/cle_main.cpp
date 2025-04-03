#include <QtWidgets/QStyleFactory>

#include "cle_main.h"

/* these must last for the lifetime of the QApplication */
static int app_argc = 1;
static char app_name[] = "retroarch";
static char *app_argv[] = { app_name, nullptr };

extern "C"
{
void cle_init()
{
  m_Application = new QApplication(app_argc, app_argv);

  m_Application->setStyle(QStyleFactory::create("Fusion"));

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

  m_Application->setPalette(darkPalette);
  m_Application->setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");

  m_MemoryInspector = new CleMemoryInspector();
  m_MemoryInspector->show();

  m_ScriptEditor = new CleScriptEditorBlock();
  m_ScriptEditor->show();
}

void cle_run()
{
   /* TODO */
}
}
