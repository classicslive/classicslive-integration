#ifndef CLE_ACTION_BLOCK_API_H
#define CLE_ACTION_BLOCK_API_H

#include "cle_action_block.h"

#include <QLabel>
#include <QLineEdit>

class CleActionBlockApi : public CleActionBlock
{
  Q_OBJECT

public:
  CleActionBlockApi(int type, QWidget* parent);

  virtual void setType(int type) override;

  virtual QString toString(void) override;

private:
  QLabel *m_Label;

  QImage *m_Image;

  /**
   * @todo replace with combobox of possible ach/ldb
   */
  QLineEdit *m_Index;

private slots:
  void onIndexEdited(const QString& text);
};

#endif
