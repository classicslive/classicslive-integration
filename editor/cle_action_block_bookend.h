#ifndef CLE_ACTION_BLOCK_BOOKEND_H
#define CLE_ACTION_BLOCK_BOOKEND_H

#include "cle_action_block.h"

#include <QLabel>

class CleActionBlockBookend : public CleActionBlock
{
  Q_OBJECT

public:
  CleActionBlockBookend(bool is_end, QWidget *parent);

  cle_result_t toString(void) override {
    return { QString("Critical error %1").arg(__FILE__), false }; }

  int indentation(void) override { return 0; }

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
