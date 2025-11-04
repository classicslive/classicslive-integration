#ifndef CLE_ACTION_BLOCK_CTRUNARY_H
#define CLE_ACTION_BLOCK_CTRUNARY_H

#include "cle_action_block.h"

#include <QLabel>
#include <QLineEdit>

class CleActionBlockCtrUnary : public CleActionBlock
{
  Q_OBJECT

public:
  CleActionBlockCtrUnary(cl_action_t *action, QWidget* parent);

  virtual void setType(cl_action_id type) override;

  virtual cle_result_t toString(void) override;

private:
  QLabel *m_Label;
  QLineEdit *m_CounterIndex;
};

#endif
