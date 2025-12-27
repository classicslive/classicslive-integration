#ifndef CLE_RESULT_TABLE_NORMAL_H
#define CLE_RESULT_TABLE_NORMAL_H

#include <QWidget>

extern "C"
{
  #include "../cl_memory.h"
  #include "../cl_search_new.h"
}

#include "cle_result_table.h"

/* TODO: Move this to a config option? */
#define CLE_SEARCH_MAX_ROWS 1000

class CleResultTableNormal : public CleResultTable
{
  Q_OBJECT

public:
  CleResultTableNormal(QWidget* parent);
  ~CleResultTableNormal() override;

  cl_addr_t getClickedResultAddress() override;
  void *searchData(void) override;
  int isInitted(void) override { return true; }
  cl_error rebuild(void) override;
  cl_error reset(void) override;
  cl_error run(void) override;
  QString statusString(void) override;
  cl_error step(void) override;

  cl_compare_type compareType(void) override
  {
    return m_Search.params.compare_type;
  }

  cl_value_type valueType(void) override
  {
    return m_Search.params.value_type;
  }

  cl_error setCompareType(const cl_compare_type type) override
  {
    return cl_search_change_compare_type(&m_Search, type);
  }

  cl_error setTarget(const QString& target) override
  {
    if (target.isEmpty())
      return cl_search_change_target(&m_Search, NULL);
    else switch (m_Search.params.value_type)
    {
    case CL_MEMTYPE_INT8:
    {
      int8_t val = (int8_t)target.toInt();
      return cl_search_change_target(&m_Search, &val);
    }
    case CL_MEMTYPE_UINT8:
    {
      uint8_t val = (uint8_t)target.toUInt();
      return cl_search_change_target(&m_Search, &val);
    }
    case CL_MEMTYPE_INT16:
    {
      int16_t val = (int16_t)target.toInt();
      return cl_search_change_target(&m_Search, &val);
    }
    case CL_MEMTYPE_UINT16:
    {
      uint16_t val = (uint16_t)target.toUInt();
      return cl_search_change_target(&m_Search, &val);
    }
    case CL_MEMTYPE_INT32:
    {
      int32_t val = (int32_t)target.toInt();
      return cl_search_change_target(&m_Search, &val);
    }
    case CL_MEMTYPE_UINT32:
    {
      uint32_t val = (uint32_t)target.toUInt();
      return cl_search_change_target(&m_Search, &val);
    }
    case CL_MEMTYPE_INT64:
    {
      int64_t val = (int64_t)target.toLongLong();
      return cl_search_change_target(&m_Search, &val);
    }
    case CL_MEMTYPE_DOUBLE:
    {
      double val = target.toDouble();
      return cl_search_change_target(&m_Search, &val);
    }
    case CL_MEMTYPE_FLOAT:
    {
      float val = target.toFloat();
      return cl_search_change_target(&m_Search, &val);
    }
    case CL_MEMTYPE_NOT_SET:
    case CL_MEMTYPE_SIZE:
      break;
    }

    return CL_ERR_PARAMETER_INVALID;
  }

  cl_error setValueType(const cl_value_type type) override
  {
    return cl_search_change_value_type(&m_Search, type);
  }

public slots:
  void onClickResultAddMemoryNote(void);
  void onClickResultPointerSearch(void);
  void onClickResultRemove(void);
  void onResultClick(QTableWidgetItem *item) override;
  void onResultDoubleClick(void) override;
  void onResultEdited(QTableWidgetItem *item) override;
  void onResultRightClick(const QPoint&) override;
  void onResultSelectionChanged(void) override;

signals:
  void addressChanged(cl_addr_t address);
  void requestAddMemoryNote(cl_memnote_t note);
  void requestPointerSearch(cl_addr_t address);

private:
  cl_search_t m_Search;
};

#endif
