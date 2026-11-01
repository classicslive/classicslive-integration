#ifndef CLE_ACTIVE_REGIONS_DIALOG_H
#define CLE_ACTIVE_REGIONS_DIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QDialogButtonBox>

extern "C"
{
  #include "../cl_memory.h"
}

class CleActiveRegionsDialog : public QDialog
{
  Q_OBJECT

public:
  CleActiveRegionsDialog(QWidget *parent = nullptr);

private slots:
  void onItemChanged(QListWidgetItem *item);

private:
  QListWidget     *m_List;
  QVBoxLayout     *m_Layout;
};

#endif
