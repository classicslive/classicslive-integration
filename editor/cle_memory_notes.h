#pragma once

#include <QWidget>
#include <QTableWidget>

extern "C"
{
  #include "../cl_types.h"
}

class CleMemoryNotes : public QWidget
{
  Q_OBJECT

public:
  explicit CleMemoryNotes(QWidget* parent = nullptr);
  ~CleMemoryNotes();

  void rebuild(void);

  void update(void);

signals:
  void addressActivated(cl_addr_t address);
  void noteActivated(cl_memnote_t note);

private:
  
private slots:
  void onItemDoubleClicked(QTableWidgetItem *item);

private:
  QTableWidget* m_Table;
};
