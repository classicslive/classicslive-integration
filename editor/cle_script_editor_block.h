#ifndef CLE_SCRIPT_EDITOR_BLOCK_H
#define CLE_SCRIPT_EDITOR_BLOCK_H

#include <QPushButton>
#include <QScrollBar>
#include <QTabWidget>

#include "cle_script_editor_block_canvas.h"

class CleScriptEditorBlock : public QWidget
{
  Q_OBJECT

public:
  CleScriptEditorBlock(QWidget *parent = nullptr);

  void rebuild(void) { m_Canvas->buildFromScript(); }
  QString script(void) { return m_Script; }

private:
  CleScriptEditorBlockCanvas *m_Canvas = nullptr;
  QPushButton *m_SaveButton = nullptr;
  QPushButton *m_SubmitButton = nullptr;
  QString m_Script;

private slots:
  void onSaveButtonClicked(void);
};

#endif
