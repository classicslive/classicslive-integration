#include "cle_active_regions_dialog.h"

CleActiveRegionsDialog::CleActiveRegionsDialog(QWidget *parent)
  : QDialog(parent)
{
  setWindowTitle(tr("Active Regions"));
  setMinimumWidth(300);

  m_List = new QListWidget(this);

  for (unsigned i = 0; i < memory.region_count; i++)
  {
    const cl_memory_region_t *region = &memory.regions[i];
    const char *title = region->title;

    QString label = (title && title[0])
      ? QString(title)
      : QString("0x%1").arg(region->base_guest, 0, 16);

    QListWidgetItem *item = new QListWidgetItem(label, m_List);
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    item->setCheckState(region->ignore ? Qt::Unchecked : Qt::Checked);
    item->setData(Qt::UserRole, i);
  }

  connect(m_List, &QListWidget::itemChanged,
          this,   &CleActiveRegionsDialog::onItemChanged);

  QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::accept);

  m_Layout = new QVBoxLayout(this);
  m_Layout->addWidget(m_List);
  m_Layout->addWidget(buttons);
  setLayout(m_Layout);
}

void CleActiveRegionsDialog::onItemChanged(QListWidgetItem *item)
{
  unsigned index = item->data(Qt::UserRole).toUInt();
  if (index >= memory.region_count)
    return;

  memory.regions[index].ignore = (item->checkState() == Qt::Unchecked)
    ? CL_TRUE : CL_FALSE;
}
