extern "C"
{
  #include "../cl_script.h"
}

#include "cle_action_block_api.h"

CleActionBlockApi::CleActionBlockApi(int type, QWidget* parent)
  : CleActionBlock(parent)
{
  /* Operand label */
  m_Label = new QLabel(this);
  m_Layout->addWidget(m_Label);

  /* Operand value */
  m_Index = new QLineEdit(this);
  m_Index->setGeometry(0, 0, 16, CLE_BLOCK_HEIGHT);
  connect(m_Index, SIGNAL(textEdited(QString)),
          this, SLOT(onIndexEdited(QString)));
  m_Layout->addWidget(m_Index);

  m_Image = QImage(256, 256, QImage::Format_ARGB32);
  m_Image.fill(Qt::red);
  m_Image = m_Image.scaled(CLE_BLOCK_HEIGHT, CLE_BLOCK_HEIGHT);

  /* Image label */
  m_ImageLabel = new QLabel(this);
  m_ImageLabel->setPixmap(QPixmap::fromImage(m_Image));
  m_Layout->addWidget(m_ImageLabel);

  setType(type);

  setLayout(m_Layout);
}

/** @todo validate to only int */

void CleActionBlockApi::onIndexEdited(const QString& text)
{
  bool ok = false;
  unsigned index = stringToValue(text, &ok);

  if (ok)
  {
    /** @todo set new image icon */
    m_Image.fill(QColor(index % 255, 0, 0));
    m_Image = m_Image.scaled(CLE_BLOCK_HEIGHT, CLE_BLOCK_HEIGHT);
    m_ImageLabel->setPixmap(QPixmap::fromImage(m_Image));
  }
}

void CleActionBlockApi::setType(int type)
{
  switch (type)
  {
  case CL_ACTTYPE_POST_ACHIEVEMENT:
    m_Label->setText("Unlock achievement");
    break;
  case CL_ACTTYPE_POST_LEADERBOARD:
    m_Label->setText("Post entry to leaderboard");
    break;
  case CL_ACTTYPE_POST_PROGRESS:
    m_Label->setText("Post progress for achievement");
    break;
  case CL_ACTTYPE_POST_POLL:
    m_Label->setText("Submit poll");
    break;
  default:
    m_Label->setText("Invalid API action " + QString::number(m_Type));
  }
  CleActionBlock::setType(type);
}

cle_result_t CleActionBlockApi::toString(void)
{
  bool ok = false;
  QString string = QString("%1 %2 2 %3 %4")
    .arg(m_Indentation, 0, CL_RADIX)
    .arg(m_Type, 0, CL_RADIX)
    .arg(CL_SRCTYPE_IMMEDIATE_INT) /** @todo programmatic index */
    .arg(stringToValue(m_Index->text(), &ok), 0, CL_RADIX);

  if (ok)
    return { string, true };
  else
    return { "Empty or invalid values", false };
}
