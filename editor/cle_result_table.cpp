#include <QHeaderView>

#include "cle_common.h"
#include "cle_result_table.h"

extern "C"
{
  #include "../cl_memory.h"
}

QTableWidget* CleResultTable::table(void)
{
  return m_Table;
}

cl_error CleResultTable::init(void)
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

  return CL_OK;
}

cl_error CleResultTable::writeMemory(const cl_addr_t address,
  const cl_search_parameters_t& params, const QString& string)
{
  bool ok;
  int64_t value64 = stringToValue(string, &ok);

  if (!ok)
    return CL_ERR_PARAMETER_INVALID;

  switch (params.value_type)
  {
  case CL_MEMTYPE_UINT8:
    {
      uint8_t value = static_cast<uint8_t>(value64);
      cl_write_memory_value(&value, nullptr, address, params.value_type);
    }
    break;
  case CL_MEMTYPE_UINT16:
    {
      uint16_t value = static_cast<uint16_t>(value64);
      cl_write_memory_value(&value, nullptr, address, params.value_type);
    }
    break;
  case CL_MEMTYPE_UINT32:
    {
      uint32_t value = static_cast<uint32_t>(value64);
      cl_write_memory_value(&value, nullptr, address, params.value_type);
    }
    break;
  case CL_MEMTYPE_INT8:
    {
      int8_t value = static_cast<int8_t>(value64);
      cl_write_memory_value(&value, nullptr, address, params.value_type);
    }
    break;
  case CL_MEMTYPE_INT16:
    {
      int16_t value = static_cast<int16_t>(value64);
      cl_write_memory_value(&value, nullptr, address, params.value_type);
    }
    break;
  case CL_MEMTYPE_INT32:
    {
      int32_t value = static_cast<int32_t>(value64);
      cl_write_memory_value(&value, nullptr, address, params.value_type);
    }
    break;
  case CL_MEMTYPE_INT64:
    {
      int64_t value = static_cast<int64_t>(value64);
      cl_write_memory_value(&value, nullptr, address, params.value_type);
    }
    break;
  case CL_MEMTYPE_FLOAT:
    {
      float value;
      uint32_t temp = static_cast<uint32_t>(value64);
      memcpy(&value, &temp, sizeof(float));
      cl_write_memory_value(&value, nullptr, address, params.value_type);
    }
    break;
  case CL_MEMTYPE_DOUBLE:
    {
      double value;
      uint64_t temp = static_cast<uint64_t>(value64);
      memcpy(&value, &temp, sizeof(double));
      cl_write_memory_value(&value, nullptr, address, params.value_type);
    }
    break;
  default:
    return CL_ERR_PARAMETER_INVALID;
  }
}
