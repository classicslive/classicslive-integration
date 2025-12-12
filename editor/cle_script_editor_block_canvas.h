#ifndef CLE_SCRIPT_EDITOR_BLOCK_CANVAS_H
#define CLE_SCRIPT_EDITOR_BLOCK_CANVAS_H

#include <QPushButton>
#include <QWidget>
#include <vector>

#include "cle_action_block.h"

class CleScriptEditorBlockCanvas : public QWidget
{
  Q_OBJECT

public:
  CleScriptEditorBlockCanvas(QWidget *parent = nullptr);

  cl_error buildFromScript(void);
  cl_error buildNew(void);

  cle_result_t toString(void);

public slots:
  void checkSnaps(CleActionBlock* position);

  /* QWidget overrides */
  void mousePressEvent(QMouseEvent *event) override;

protected:
  void paintEvent(QPaintEvent *event) override;

private:
  std::vector<CleActionBlock*> blocks;

  CleActionBlock *addBlock(cl_action_t *action);
  CleActionBlock *addBlock(cl_action_t *action, QPoint pos);
  void destroyBlocks(void);
};

#endif
