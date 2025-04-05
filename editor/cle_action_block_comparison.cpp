extern "C"
{
  #include "../cl_script.h"
}

#include "cle_action_block_comparison.h"

CleActionBlockComparison::CleActionBlockComparison(cl_action_t *action,
  QWidget* parent = nullptr) : CleActionBlock(action, parent)
{
  /* Label */
  m_Label = new QLabel(this);
  m_Label->setText("If");
  m_Layout->addWidget(m_Label);

  /* Left operand type */
  m_LeftType = new QComboBox(this);
  m_LeftType->addItem("current", CL_SRCTYPE_CURRENT_RAM);
  m_LeftType->addItem("previous", CL_SRCTYPE_PREVIOUS_RAM);
  m_LeftType->addItem("last unique", CL_SRCTYPE_LAST_UNIQUE_RAM);
  m_Layout->addWidget(m_LeftType);

  /* Left operand value (dropdown box) */
  /* TODO: Have this refresh when the combobox is clicked */
  m_LeftComboBox = new QComboBox(this);
  for (unsigned i = 0; i < memory.note_count; i++)
    m_LeftComboBox->addItem(memory.notes[i].title);
  m_Layout->addWidget(m_LeftComboBox);

  /* Comparison selector */
  m_ComparisonType = new QComboBox(this);
  m_ComparisonType->addItem("＝ is equal to", CL_CMPTYPE_IFEQUAL);
  m_ComparisonType->addItem("!= is not equal to", CL_CMPTYPE_IFNEQUAL);
  m_ComparisonType->addItem("＜ is less than", CL_CMPTYPE_IFLESS);
  m_ComparisonType->addItem("＞ is greater than", CL_CMPTYPE_IFGREATER);
  m_Layout->addWidget(m_ComparisonType);

  /* Right operand type */
  m_RightType = new QComboBox(this);
  m_RightType->addItem("constant", CL_SRCTYPE_IMMEDIATE_INT);
  m_RightType->addItem("current", CL_SRCTYPE_CURRENT_RAM);
  m_RightType->addItem("previous", CL_SRCTYPE_PREVIOUS_RAM);
  m_RightType->addItem("last unique", CL_SRCTYPE_LAST_UNIQUE_RAM);
  connect(m_RightType, SIGNAL(currentIndexChanged(int)), this, SLOT(onChangeRightType(int)));
  m_Layout->addWidget(m_RightType);

  /* Right operand value (text entry) */
  m_RightLineEdit = new QLineEdit(this);

  /* Right operand value (dropdown box) */
  /* TODO: Have this refresh when the combobox is clicked */
  m_RightComboBox = new QComboBox(this);
  for (unsigned i = 0; i < memory.note_count; i++)
    m_RightComboBox->addItem(memory.notes[i].title);

  /* Right operand value stacker */
  m_RightStack = new QStackedWidget(this);
  m_RightStack->addWidget(m_RightLineEdit);
  m_RightStack->addWidget(m_RightComboBox);
  m_Layout->addWidget(m_RightStack);

  setType(type());
  populate();

  setLayout(m_Layout);
}

void CleActionBlockComparison::onChangeRightType(int index)
{
  Q_UNUSED(index)
  switch (m_RightType->currentData().toUInt())
  {
  /* Use the QLineEdit */
  case CL_SRCTYPE_IMMEDIATE_INT:
  case CL_SRCTYPE_COUNTER:
    m_RightStack->setCurrentIndex(0);
    break;

  /* Use the QComboBox */
  case CL_SRCTYPE_CURRENT_RAM:
  case CL_SRCTYPE_PREVIOUS_RAM:
  case CL_SRCTYPE_LAST_UNIQUE_RAM:
    m_RightStack->setCurrentIndex(1);
    break;
  }
}

void CleActionBlockComparison::populate(void)
{
  m_LeftType->setCurrentIndex(
    m_LeftType->findData(m_Action->arguments[0].uintval));

  m_LeftComboBox->setCurrentIndex(
    m_LeftComboBox->findData(m_Action->arguments[1].uintval));

  m_ComparisonType->setCurrentIndex(
    m_ComparisonType->findData(m_Action->arguments[4].uintval));

  m_RightType->setCurrentIndex(
    m_RightType->findData(m_Action->arguments[2].uintval));
  onChangeRightType(0);

  if (m_RightStack->currentIndex() == 0)
    m_RightLineEdit->setText(QString::number(m_Action->arguments[3].uintval));
  else
    m_RightType->setCurrentIndex(
      m_RightComboBox->findData(m_Action->arguments[3].uintval));
}

int64_t CleActionBlockComparison::rightValue(void)
{
  switch (m_RightType->currentData().toUInt())
  {
  case CL_SRCTYPE_IMMEDIATE_INT:
  case CL_SRCTYPE_COUNTER:
    return stringToValue(m_RightLineEdit->text(), nullptr);
  case CL_SRCTYPE_CURRENT_RAM:
  case CL_SRCTYPE_PREVIOUS_RAM:
  case CL_SRCTYPE_LAST_UNIQUE_RAM:
    return m_RightComboBox->currentData().toUInt();
  }

  return 0;
}

void CleActionBlockComparison::setType(int type)
{
  switch (type)
  {
  case CL_ACTTYPE_COMPARE:
    m_Label->setText("If");
    break;
  default:
    m_Label->setText("Invalid comparison action " + QString::number(type));
  }
  CleActionBlock::setType(type);
}

cle_result_t CleActionBlockComparison::toString(void)
{
  QString string = QString("%1 %2 5 %3 %4 %5 %6 %7")
    .arg(indentation(), 0, CL_RADIX)
    .arg(type(), 0, CL_RADIX)
    .arg(m_LeftType->currentData().toUInt(), 0, CL_RADIX)
    .arg(m_LeftComboBox->currentData().toUInt(), 0, CL_RADIX)
    .arg(m_ComparisonType->currentData().toUInt(), 0, CL_RADIX)
    .arg(m_RightType->currentData().toUInt(), 0, CL_RADIX)
    .arg(rightValue(), 0, CL_RADIX);

  return { string, true };
}
