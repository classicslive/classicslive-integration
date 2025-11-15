extern "C"
{
  #include "../cl_script.h"
}

#include "cle_action_block_ctrbinary.h"

CleActionBlockCtrBinary::CleActionBlockCtrBinary(cl_action_t *action,
  QWidget* parent = nullptr) : CleActionBlock(action, parent)
{
  /* Left operand label */
  m_LabelA = new QLabel(this);
  m_Layout->addWidget(m_LabelA);

  /* Left operand value */
  m_CounterIndex = new QLineEdit(this);
  m_CounterIndex->setGeometry(0, 0, 16, CLE_BLOCK_HEIGHT);
  m_Layout->addWidget(m_CounterIndex);

  /* Right operand label */
  m_LabelB = new QLabel(this);
  m_Layout->addWidget(m_LabelB);

  /* Right operand type */
  m_ModifierType = new QComboBox(this);
  m_ModifierType->addItem("constant", CL_SRCTYPE_IMMEDIATE_INT);
  m_ModifierType->addItem("current", CL_SRCTYPE_CURRENT_RAM);
  m_ModifierType->addItem("previous", CL_SRCTYPE_PREVIOUS_RAM);
  m_ModifierType->addItem("last unique", CL_SRCTYPE_LAST_UNIQUE_RAM);
  connect(m_ModifierType, SIGNAL(currentIndexChanged(int)),
          this, SLOT(onChangeModifierType(int)));
  m_Layout->addWidget(m_ModifierType);

  /* Right operand value (text entry) */
  m_ModifierValueLineEdit = new QLineEdit(this);

  /* Right operand value (dropdown box) */
  /* TODO: Have this refresh when the combobox is clicked */
  m_ModifierValueComboBox = new QComboBox(this);
  for (unsigned i = 0; i < memory.note_count; i++)
    m_ModifierValueComboBox->addItem(memory.notes[i].details.title);

  /* Right operand value stacker */
  m_ModifierStack = new QStackedWidget(this);
  m_ModifierStack->addWidget(m_ModifierValueLineEdit);
  m_ModifierStack->addWidget(m_ModifierValueComboBox);
  m_Layout->addWidget(m_ModifierStack);

  setLayout(m_Layout);
}

int64_t CleActionBlockCtrBinary::modifierValue(void)
{
  switch (m_ModifierType->currentData().toUInt())
  {
  case CL_SRCTYPE_IMMEDIATE_INT:
  case CL_SRCTYPE_COUNTER:
    return stringToValue(m_ModifierValueLineEdit->text(), nullptr);
  case CL_SRCTYPE_CURRENT_RAM:
  case CL_SRCTYPE_PREVIOUS_RAM:
  case CL_SRCTYPE_LAST_UNIQUE_RAM:
    return m_ModifierValueComboBox->currentData().toUInt();
  }

  return 0;
}

void CleActionBlockCtrBinary::onChangeModifierType(int index)
{
  Q_UNUSED(index)
  switch (m_ModifierType->currentData().toUInt())
  {
  /* Use the QLineEdit */
  case CL_SRCTYPE_IMMEDIATE_INT:
  case CL_SRCTYPE_COUNTER:
    m_ModifierStack->setCurrentIndex(0);
    break;
  
  /* Use the QComboBox */
  case CL_SRCTYPE_CURRENT_RAM:
  case CL_SRCTYPE_PREVIOUS_RAM:
  case CL_SRCTYPE_LAST_UNIQUE_RAM:
    m_ModifierStack->setCurrentIndex(1);
    break;
  }
}

void CleActionBlockCtrBinary::setType(cl_action_id type)
{
  switch (type)
  {
  case CL_ACTTYPE_AND:
    m_LabelA->setText("Bitwise AND counter");
    m_LabelB->setText("with");
    break;
  case CL_ACTTYPE_OR:
    m_LabelA->setText("Bitwise OR counter");
    m_LabelB->setText("with");
    break;
  case CL_ACTTYPE_XOR:
    m_LabelA->setText("Bitwise XOR counter");
    m_LabelB->setText("with");
    break;
  case CL_ACTTYPE_MULTIPLICATION:
    m_LabelA->setText("Multiply counter");
    m_LabelB->setText("by");
    break;
  case CL_ACTTYPE_ADDITION:
    m_LabelA->setText("Add counter");
    m_LabelB->setText("to");
    break;
  case CL_ACTTYPE_SHIFT_LEFT:
    m_LabelA->setText("Left shift counter");
    m_LabelB->setText("by");
    break;
  case CL_ACTTYPE_SHIFT_RIGHT:
    m_LabelA->setText("Right shift counter");
    m_LabelB->setText("by");
    break;
  case CL_ACTTYPE_SET:
    m_LabelA->setText("Set counter");
    m_LabelB->setText("to");
    break;
  default:
    m_LabelA->setText("Invalid ctrbinary action " + QString::number(type));
  }
  CleActionBlock::setType(type);
}

cle_result_t CleActionBlockCtrBinary::toString(void)
{
  bool ok;
  QString string = QString("%1 %2 3 %3 %4 %5")
    .arg(indentation(), 0, CL_RADIX)
    .arg(type(), 0, CL_RADIX)
    .arg(stringToValue(m_CounterIndex->text(), &ok), 0, CL_RADIX)
    .arg(m_ModifierType->currentData().toUInt(), 0, CL_RADIX)
    .arg(modifierValue(), 0, CL_RADIX);

  if (ok)
    return { string, true };
  else
    return { "Empty or invalid values", false };
}
