#ifndef CLE_ACTION_BLOCK_API_H
#define CLE_ACTION_BLOCK_API_H

#include "cle_action_block.h"

#include <QComboBox>
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
  QLabel *m_Label = nullptr;

  QLabel *m_ImageLabel = nullptr;

  QImage m_Image;

  QComboBox *m_ComboBox = nullptr;

  void setAchievementIconByIndex(unsigned index);

private slots:
  void onChanged(int index);
  void onEdited(const QString& text);
};

#endif
