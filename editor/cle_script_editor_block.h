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

private:
  CleScriptEditorBlockCanvas *m_Canvas;
  QPushButton *m_SaveButton;

private slots:
  void onSaveButtonClicked(void);
};

#endif
