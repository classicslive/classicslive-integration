#ifndef CLE_ACTION_BLOCK_API_H
#define CLE_ACTION_BLOCK_API_H

#include "cle_action_block.h"

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QStackedWidget>

/* Pages of the value stack, one per kind of ID source */
typedef enum
{
  CLE_API_VALUE_CONSTANT = 0,
  CLE_API_VALUE_COUNTER,
  CLE_API_VALUE_NOTE
} cle_api_value_page;

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

  /* Where the ID comes from: a constant, a counter, or a memory note */
  QComboBox *m_SourceType = nullptr;

  /* Counter index, when the ID comes from a counter */
  QLineEdit *m_CounterEdit = nullptr;

  /* Memory note, when the ID comes from a note's value */
  QComboBox *m_NoteComboBox = nullptr;

  /* Holds the value widget for each source type */
  QStackedWidget *m_ValueStack = nullptr;

  void setAchievementIconByIndex(unsigned index);

  /** @return The ID for a constant source, or 0 with ok false if invalid */
  unsigned constantId(bool *ok);

private slots:
  void onChanged(int index);
  void onChangeSource(int index);
  void onEdited(const QString& text);
};

#endif
