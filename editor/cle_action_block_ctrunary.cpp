extern "C"
{
  #include "../cl_script.h"
}

#include "cle_action_block_ctrunary.h"

CleActionBlockCtrUnary::CleActionBlockCtrUnary(int type, QWidget* parent)
{
  /* Operand label */
  m_Label = new QLabel(this);
  m_Layout->addWidget(m_Label);

  /* Operand value */
  m_CounterIndex = new QLineEdit(this);
  m_CounterIndex->setGeometry(0, 0, 16, CLE_BLOCK_HEIGHT);
  m_Layout->addWidget(m_CounterIndex);

  setType(type);

  setLayout(m_Layout);
}

void CleActionBlockCtrUnary:setType(int type)
{
  switch (type)
  {
  case CL_ACTTYPE_COMPLEMENT:
    m_Label->setText("Complement counter");
    break;
  default:
    m_Label->setText("Invalid action type " + QString::number(m_Type));
  }
}

QString CleActionBlockCtrUnary::toString(void)
{
  bool ok = false;
  QString string = QString("%1 %2 1 %3 ")
    .arg(m_Indentation, 0, CL_RADIX)
    .arg(m_Type, 0, CL_RADIX)
    .arg(stringToValue(m_CounterIndex->text(), &ok), 0, CL_RADIX);
  
  return ok ? string : toNopString();
}
