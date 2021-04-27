#ifndef CLE_ACTION_BLOCK_BOOKEND_H
#define CLE_ACTION_BLOCK_BOOKEND_H

#include "cle_action_block.h"

#include <QLabel>

class CleActionBlockBookend : public CleActionBlock
{
  Q_OBJECT

public:
  CleActionBlockBookend(QWidget* parent);

  virtual QString toString() override { return ""; };

  virtual bool isStart() override { return !m_IsEnd; }
  virtual bool isEnd() override { return m_IsEnd; }

  virtual void paintEvent(QPaintEvent *e) override;

private:
  /* Whether or not this bookend represents the end of a script. */
  bool m_IsEnd;

  QLabel *m_Label;
};

#endif
