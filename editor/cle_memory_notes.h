#pragma once

#include <QWidget>
#include <QTableWidget>

class CleMemoryNotes : public QWidget
{
  Q_OBJECT

public:
  explicit CleMemoryNotes(QWidget* parent = nullptr);
  ~CleMemoryNotes();

  void rebuild(void);

  void update(void);

private:
  QTableWidget* m_Table;
};
