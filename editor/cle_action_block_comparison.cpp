#include "cle_action_block_comparison.h"

extern "C"
{
  #include "../cl_memory.h"
}

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
    m_LeftComboBox->addItem(memory.notes[i].details.title, memory.notes[i].key);
  connect(m_LeftComboBox, SIGNAL(currentIndexChanged(int)),
          this, SLOT(onChangeLeftNote(int)));
  m_Layout->addWidget(m_LeftComboBox);

  /* Comparison selector */
  m_ComparisonType = new QComboBox(this);
  m_ComparisonType->addItem("＝ is equal to", CL_COMPARE_EQUAL);
  m_ComparisonType->addItem("!= is not equal to", CL_COMPARE_NOT_EQUAL);
  m_ComparisonType->addItem("＜ is less than", CL_COMPARE_LESS);
  m_ComparisonType->addItem("＞ is greater than", CL_COMPARE_GREATER);
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
  m_RightComboBoxNotes = new QComboBox(this);
  for (unsigned i = 0; i < memory.note_count; i++)
    m_RightComboBoxNotes->addItem(memory.notes[i].details.title);

  /* Right operand value (memory note friendly value selection) */
  m_RightComboBoxValues = new QComboBox(this);
  m_RightComboBoxValues->setEditable(true);

  /* Right operand value stacker */
  m_RightStack = new QStackedWidget(this);
  m_RightStack->addWidget(m_RightLineEdit);
  m_RightStack->addWidget(m_RightComboBoxNotes);
  m_RightStack->addWidget(m_RightComboBoxValues);
  m_Layout->addWidget(m_RightStack);

  if (!m_Action->arguments)
  {
    m_Action->argument_count = 5;
    m_Action->arguments = (cl_arg_t*)calloc(m_Action->argument_count, sizeof(cl_arg_t));
    m_Owned = true;
  }

  setLayout(m_Layout);
}

void CleActionBlockComparison::onChangeLeftNote(int index)
{
  Q_UNUSED(index)
  if (m_RightType->currentData().toUInt() == CL_SRCTYPE_IMMEDIATE_INT)
    onChangeRightType(0);
}

void CleActionBlockComparison::onChangeRightType(int index)
{
  Q_UNUSED(index)
  uint type = m_RightType->currentData().toUInt();

  switch (type)
  {
  case CL_SRCTYPE_IMMEDIATE_INT:
  {
    // Check if the left operand references a note with friendly values
    int leftNoteIndex = m_LeftComboBox->currentIndex();
    if (leftNoteIndex < 0 || leftNoteIndex >= (int)memory.note_count)
    {
      // No valid note — fallback to plain line edit
      m_RightStack->setCurrentIndex(0);
      return;
    }

    cl_memnote_t* note = &memory.notes[leftNoteIndex];
    bool hasFriendlyValues = note->details.values[0].title[0] != '\0';

    if (hasFriendlyValues)
    {
      // Prepare combo box with friendly values
      m_RightComboBoxValues->clear();
      for (unsigned i = 0; i < 64 && note->details.values[i].title[0]; i++)
      {
        const auto& v = note->details.values[i];
        QString label = QString("%1 (%2)").arg(v.title).arg(v.value);
        m_RightComboBoxValues->addItem(label, v.value);
      }
      m_RightStack->setCurrentIndex(2);
    }
    else
    {
      // No friendly values — just use line edit
      m_RightStack->setCurrentIndex(0);
    }
    break;
  }

  case CL_SRCTYPE_COUNTER:
    // Counters use the line edit only
    m_RightStack->setCurrentIndex(0);
    break;

  case CL_SRCTYPE_CURRENT_RAM:
  case CL_SRCTYPE_PREVIOUS_RAM:
  case CL_SRCTYPE_LAST_UNIQUE_RAM:
    // Normal memory note selection
    m_RightStack->setCurrentIndex(1);
    break;
  }
}

void CleActionBlockComparison::populate(void)
{
  m_LeftType->setCurrentIndex(
    m_LeftType->findData(static_cast<uint>(m_Action->arguments[0].uintval)));

  m_LeftComboBox->setCurrentIndex(
    m_LeftComboBox->findData(static_cast<uint>(m_Action->arguments[1].uintval)));

  m_ComparisonType->setCurrentIndex(
    m_ComparisonType->findData(static_cast<uint>(m_Action->arguments[4].uintval)));

  m_RightType->setCurrentIndex(
    m_RightType->findData(static_cast<uint>(m_Action->arguments[2].uintval)));

  // Refresh UI for correct right-side type
  onChangeRightType(0);

  uint rightType = m_RightType->currentData().toUInt();
  uint rightValue = static_cast<uint>(m_Action->arguments[3].uintval);
  int idx;

  switch (rightType)
  {
  case CL_SRCTYPE_IMMEDIATE_INT:
  case CL_SRCTYPE_COUNTER:
    if (m_RightStack->currentIndex() == 2)
    {
      // friendly-value combo box (editable)
      idx = m_RightComboBoxValues->findData(rightValue);
      if (idx >= 0)
        m_RightComboBoxValues->setCurrentIndex(idx);
      else
        m_RightComboBoxValues->setEditText(QString::number(rightValue));
    }
    else
      m_RightLineEdit->setText(QString::number(rightValue));
    break;

  case CL_SRCTYPE_CURRENT_RAM:
  case CL_SRCTYPE_PREVIOUS_RAM:
  case CL_SRCTYPE_LAST_UNIQUE_RAM:
    idx = m_RightComboBoxNotes->findData(rightValue);
    if (idx >= 0)
      m_RightComboBoxNotes->setCurrentIndex(idx);
    break;
  }
}


int64_t CleActionBlockComparison::rightValue(void)
{
  uint type = m_RightType->currentData().toUInt();

  switch (type)
  {
  case CL_SRCTYPE_IMMEDIATE_INT:
  case CL_SRCTYPE_COUNTER:
    switch (m_RightStack->currentIndex())
    {
    case 0:
      return stringToValue(m_RightLineEdit->text(), nullptr);
    case 2:
      if (m_RightComboBoxValues->currentIndex() >= 0)
        return m_RightComboBoxValues->currentData().toLongLong();
      else
        return stringToValue(m_RightComboBoxValues->currentText(), nullptr);
    default:
      return 0;
    }
  case CL_SRCTYPE_CURRENT_RAM:
  case CL_SRCTYPE_PREVIOUS_RAM:
  case CL_SRCTYPE_LAST_UNIQUE_RAM:
    return m_RightComboBoxNotes->currentData().toUInt();
  default:
    return 0;
  }
}

void CleActionBlockComparison::setType(cl_action_id type)
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
    .arg(m_RightType->currentData().toUInt(), 0, CL_RADIX)
    .arg(rightValue(), 0, CL_RADIX)
    .arg(m_ComparisonType->currentData().toUInt(), 0, CL_RADIX);

  return { string, true };
}
