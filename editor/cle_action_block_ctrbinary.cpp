#ifndef CLE_ACTION_BLOCK_CTRBINARY_CPP
#define CLE_ACTION_BLOCK_CTRBINARY_CPP

#include "cle_action_block_ctrbinary.h"

CleActionBlockCtrBinary::CleActionBlockCtrBinary(QWidget* parent) : CleActionBlock(parent)
{
  setParent(parent);
  m_LabelA = new QLabel("Modify counter", this);
  m_Layout->addWidget(m_LabelA);

  m_CounterIndex = new QLineEdit(this);
  m_CounterIndex->setGeometry(0, 0, 16, CLE_BLOCK_HEIGHT);
  m_Layout->addWidget(m_CounterIndex);

  m_LabelB = new QLabel("with", this);
  m_Layout->addWidget(m_LabelB);

  m_ModifierType = new QComboBox(this);
  m_ModifierType->addItem("constant",    CL_SRCTYPE_IMMEDIATE);
  m_ModifierType->addItem("previous",    CL_SRCTYPE_PREVIOUS_RAM);
  m_ModifierType->addItem("current",     CL_SRCTYPE_CURRENT_RAM);
  m_ModifierType->addItem("last unique", CL_SRCTYPE_LAST_UNIQUE_RAM);
  connect(m_ModifierType, SIGNAL(currentIndexChanged(int)), this, SLOT(onChangeModifierType(int)));
  m_Layout->addWidget(m_ModifierType);

  m_ModifierValueLineEdit = new QLineEdit(this);

  m_ModifierValueComboBox = new QComboBox(this);
  m_ModifierValueComboBox->addItem(memory.notes[0].title);

  m_ModifierStack = new QStackedWidget(this);
  m_ModifierStack->addWidget(m_ModifierValueLineEdit);
  m_ModifierStack->addWidget(m_ModifierValueComboBox);
  m_Layout->addWidget(m_ModifierStack);

  setLayout(m_Layout);
}

uint32_t CleActionBlockCtrBinary::getModifierValue()
{
  switch (m_ModifierType->currentData().toUInt())
  {
  case CL_SRCTYPE_IMMEDIATE:
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
  case CL_SRCTYPE_IMMEDIATE:
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

void CleActionBlockCtrBinary::setType(uint8_t type)
{
  /*
  switch (type)
  {
  case CL_ACTTYPE_AND:
    m_LabelA->setText("Logical AND counter");
    break;
  default:
    m_LabelA->setText("Invalid action type " + QString::number(m_Type));
  }
  */
}

QString CleActionBlockCtrBinary::toString()
{
  bool ok;
  QString string = QString
  (
    QString::number(m_Indentation) + 
    QString::number(m_Type) +
    " 3 " +
    stringToValue(m_CounterIndex->text(), &ok) + " " +
    m_ModifierType->currentData().toUInt() + " " +
    getModifierValue() + " "
  );

  return string;
}

#endif
