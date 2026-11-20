#include "cle_action_block_keyword.h"

CleActionBlockKeyword::CleActionBlockKeyword(cl_action_t *action,
  QWidget *parent = nullptr) : CleActionBlock(action, parent)
{
  m_Label = new QLabel(this);
  m_Layout->addWidget(m_Label);

  setLayout(m_Layout);
}

void CleActionBlockKeyword::setType(cl_action_id type)
{
  CleActionBlock::setType(type);
  if (type == CL_ACTTYPE_LOGICAL_OR)
    m_Label->setText(tr("OR"));
  else if (type == CL_ACTTYPE_ELSE)
    m_Label->setText(tr("Else"));
  else
    m_Label->setText(tr("Invalid action %1").arg(type));
}

cle_result_t CleActionBlockKeyword::toString(void)
{
  QString string = QString("%1 %2 0")
    .arg(indentation(), 0, CL_RADIX)
    .arg(type(), 0, CL_RADIX);

  return { string, true };
}
