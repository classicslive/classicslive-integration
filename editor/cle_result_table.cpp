#include <QHeaderView>

#include "cle_common.h"
#include "cle_result_table.h"

extern "C"
{
   #include "../cl_common.h"
}

QTableWidget* CleResultTable::getTable()
{
   return m_Table;
}

void CleResultTable::init()
{
   /* Initialize basic table properties and style */
   m_Table = new QTableWidget();
   m_Table->setRowCount(0);
   m_Table->setContextMenuPolicy(Qt::CustomContextMenu);
   m_Table->setAlternatingRowColors(true);
   m_Table->setShowGrid(false);
   m_Table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::Stretch);
   m_Table->verticalHeader()->setVisible(false);
   m_Table->verticalHeader()->setDefaultSectionSize(16);

   /* Setup Qt slots */
   connect(m_Table, SIGNAL(itemClicked(QTableWidgetItem*)),
      this, SLOT(onResultClick(QTableWidgetItem*)));
   connect(m_Table, SIGNAL(itemDoubleClicked(QTableWidgetItem*)),
      this, SLOT(onResultDoubleClick()));
   connect(m_Table, SIGNAL(itemChanged(QTableWidgetItem*)),
      this, SLOT(onResultEdited(QTableWidgetItem*)));
   connect(m_Table, SIGNAL(customContextMenuRequested(const QPoint&)),
      this, SLOT(onResultRightClick(const QPoint&)));
   connect(m_Table, SIGNAL(itemSelectionChanged()),
      this, SLOT(onResultSelectionChanged()));

   m_ClickedResult = -1;
   m_CurrentEditedRow = -1;
}

void CleResultTable::writeMemory(const cl_addr_t address,
   const cl_search_params_t& params, const QString& string)
{
   void *value;
   bool  ok;

   if (params.value_type == CL_MEMTYPE_FLOAT)
      value = new float(string.toFloat(&ok));
   else if (params.value_type == CL_MEMTYPE_DOUBLE)
      value = new double(string.toDouble(&ok));
   else
      value = new uint64_t(stringToValue(string, &ok));

   if (ok)
   {
      cl_write_memory(NULL, address, params.size, value);
      cl_log("Wrote %s to 0x%08X.\n", string.toStdString().c_str(), address);
   }
   free(value);
}
