#ifndef CLE_ACTION_BLOCK_API_H
#define CLE_ACTION_BLOCK_API_H

#include "cle_action_block.h"

#include <QLabel>
#include <QLineEdit>

class CleActionBlockApi : public CleActionBlock
{
  Q_OBJECT

public:
  CleActionBlockApi(cl_action_t *action, QWidget* parent);

  virtual void populate(void) override;

  virtual void setType(cl_action_id type) override;

  virtual cle_result_t toString(void) override;

private:
  QLabel *m_Label;

  QLabel *m_ImageLabel;

  QImage m_Image;

  /**
   * @todo replace with combobox of possible ach/ldb
   */
  QLineEdit *m_Index;

private slots:
  void onIndexEdited(const QString& text);
};

#endif
