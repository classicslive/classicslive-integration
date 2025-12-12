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

  virtual void setType(cl_action_id type) override;

  virtual cle_result_t toString(void) override;

private:
  QLabel *m_Label;
  
  /* Left operand type */
  QComboBox *m_LeftType;

  /* Left operand value (memory note selection) */
  QComboBox *m_LeftComboBox;

  /* Comparison type selection */
  QComboBox *m_ComparisonType;

  /* Right operand type selection */
  QComboBox *m_RightType;

  /* Right operand value (manually-typed immediate) */
  QLineEdit *m_RightLineEdit;

  /* Right operand value (memory note selection) */
  QComboBox *m_RightComboBoxNotes;

  /* Right operand value (memory note friendly value selection) */
  QComboBox *m_RightComboBoxValues;

  /* Holder for the different right operand value types */
  QStackedWidget *m_RightStack;

  /**
   * Returns the value of the right operand depending on the type of
   * operand selected.
   * @return The value of the right operand.
   */
  int64_t rightValue(void);

private slots:
  void onChangeLeftNote(int index);
  void onChangeRightType(int index);
};

#endif
