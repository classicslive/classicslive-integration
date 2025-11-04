#ifndef CLE_ACTION_BLOCK_NOP_H
#define CLE_ACTION_BLOCK_NOP_H

#include "cle_action_block.h"

#include <QLabel>

class CleActionBlockNop : public CleActionBlock
{
  Q_OBJECT

public:
  CleActionBlockNop(cl_action_t *action, QWidget* parent);

  virtual void setType(cl_action_id type) override;

  virtual cle_result_t toString(void) override;

private:
  QLabel *m_Label;
};

#endif
