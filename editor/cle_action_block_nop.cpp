#include "cle_action_block_nop.h"

CleActionBlockNop::CleActionBlockNop(cl_action_t *action,
  QWidget* parent = nullptr) : CleActionBlock(action, parent)
{
  /* Operand label */
  m_Label = new QLabel(this);
  m_Layout->addWidget(m_Label);

  setLayout(m_Layout);
}

void CleActionBlockNop::setType(cl_action_id type)
{
  m_Label->setText("Invalid action " + QString::number(type));
}

cle_result_t CleActionBlockNop::toString(void)
{
  return { "Invalid action.", false };
}
