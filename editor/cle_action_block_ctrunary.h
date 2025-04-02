#ifndef CLE_ACTION_BLOCK_CTRUNARY_H
#define CLE_ACTION_BLOCK_CTRUNARY_H

#include "cle_action_block.h"

#include <QLabel>
#include <QLineEdit>

class CleActionBlockCtrUnary : public CleActionBlock
{
  Q_OBJECT

public:
  CleActionBlockCtrUnary(int type, QWidget* parent);

  virtual void setType(int type) override;

  virtual QString toString(void) override;

private:
  QLabel *m_Label;
  QLineEdit *m_CounterIndex;
};

#endif
