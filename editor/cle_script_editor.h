#ifndef CLE_SCRIPT_EDITOR_H
#define CLE_SCRIPT_EDITOR_H

#include <QLabel>
#include <QTableWidget>
#include <QTimer>

#include "cle_common.h"

class CleScriptEditor : public QWidget
{
  Q_OBJECT

public:
  CleScriptEditor();

public slots:
  void run();

private:
  QTableWidget *m_Table;
  QTimer *m_UpdateTimer;
  QLabel *m_Label;
};

#endif
