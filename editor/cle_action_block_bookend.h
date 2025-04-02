#ifndef CLE_ACTION_BLOCK_BOOKEND_H
#define CLE_ACTION_BLOCK_BOOKEND_H

#include "cle_action_block.h"

#include <QLabel>

class CleActionBlockBookend : public CleActionBlock
{
  Q_OBJECT

public:
  CleActionBlockBookend(QWidget* parent, bool is_end);

  QString toString(void) override { return ""; }

  bool isStart(void) override { return !m_IsEnd; }

  bool isEnd(void) override { return m_IsEnd; }

  void paintEvent(QPaintEvent *e) override;

  int snapIndentation(QPoint pos) override;

private:
  /* Whether or not this bookend represents the end of a script. */
  bool m_IsEnd = false;

  QLabel *m_Label;
};

#endif
