#ifndef CLE_ACTION_BLOCK_CTRBINARY_H
#define CLE_ACTION_BLOCK_CTRBINARY_H

#include "cle_action_block.h"

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QStackedWidget>

class CleActionBlockCtrBinary : public CleActionBlock
{
  Q_OBJECT

public:
  CleActionBlockCtrBinary(cl_action_t *action, QWidget* parent);

  virtual void populate(void) override;

  virtual void setType(cl_action_id type) override;

  virtual cle_result_t toString(void) override;

private:
  QLabel *m_LabelA;
  QLineEdit *m_CounterIndex;
  QLabel *m_LabelB;
  
  QComboBox *m_ModifierType;
  QLineEdit *m_ModifierValueLineEdit;
  QComboBox *m_ModifierValueComboBox;
  QStackedWidget *m_ModifierStack;

  int64_t modifierValue(void);

private slots:
  void onChangeModifierType(int index);
};

#endif
