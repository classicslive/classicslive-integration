#ifndef CLE_SCRIPT_EDITOR_BLOCK_H
#define CLE_SCRIPT_EDITOR_BLOCK_H

#include <QPushButton>
#include <QWidget>
#include <vector>

#include "cle_action_block.h"

class CleScriptEditorBlock : public QWidget
{
  Q_OBJECT

public:
  CleScriptEditorBlock(QWidget *parent = nullptr);
  ~CleScriptEditorBlock();

  QString toString();

public slots:
  void checkSnaps(CleActionBlock* position);

  /* QWidget overrides */
  void mousePressEvent(QMouseEvent *event) override;

private:
  std::vector<CleActionBlock*> blocks;
};

#endif
