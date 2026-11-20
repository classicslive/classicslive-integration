#ifndef CLE_ACTION_BLOCK_KEYWORD_H
#define CLE_ACTION_BLOCK_KEYWORD_H

#include "cle_action_block.h"

#include <QLabel>

/**
 * A block for a control flow action that takes no arguments, such as OR
 * between two conditions or ELSE after a condition chain's body.
 */
class CleActionBlockKeyword : public CleActionBlock
{
  Q_OBJECT

public:
  CleActionBlockKeyword(cl_action_t *action, QWidget *parent);

  virtual void setType(cl_action_id type) override;

  virtual cle_result_t toString(void) override;

private:
  QLabel *m_Label;
};

#endif
