#ifndef CLE_ACTION_BLOCK_COMPARISON_H
#define CLE_ACTION_BLOCK_COMPARISON_H

#include "cle_action_block.h"

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QStackedWidget>

class CleActionBlockComparison : public CleActionBlock
{
  Q_OBJECT

public:
  CleActionBlockComparison(cl_action_t *action, QWidget* parent);

  virtual void populate(void) override;

  virtual void setType(int type) override;

  virtual cle_result_t toString(void) override;

private:
  QLabel *m_Label;
  
  QComboBox *m_LeftType;
  QComboBox *m_LeftComboBox;

  QComboBox *m_ComparisonType;

  QComboBox *m_RightType;
  QLineEdit *m_RightLineEdit;
  QComboBox *m_RightComboBox;
  QStackedWidget *m_RightStack;

  /**
   * Returns the value of the right operand depending on the type of
   * operand selected.
   * @return The value of the right operand.
   */
  int64_t rightValue(void);

private slots:
  void onChangeRightType(int index);
};

#endif
