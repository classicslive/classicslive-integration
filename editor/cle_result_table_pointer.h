#ifndef CLE_RESULT_TABLE_POINTER_H
#define CLE_RESULT_TABLE_POINTER_H

#include "cle_result_table.h"

extern "C"
{
  #include "../cl_search_pointer_new.h"
}

class CleResultTablePointer : public CleResultTable
{
    Q_OBJECT

public:
  CleResultTablePointer(QWidget *parent, cl_addr_t address, cl_value_type value_type,
    unsigned passes, cl_addr_t range, cl_addr_t max_results);
  ~CleResultTablePointer() override;

  cl_addr_t getClickedResultAddress() override;
  void *searchData(void) override;
  int isInitted(void) override { return true; }
  cl_error rebuild() override;
  cl_error reset(void) override;
  cl_error run(void) override;
  QString statusString(void) override { return ""; }
  cl_error step(void) override;

  cl_compare_type compareType(void) override { return m_Search.params.compare_type; }
  cl_value_type valueType(void) override { return m_Search.params.value_type; }

  cl_error setCompareType(const cl_compare_type type) override
  {
    return cl_pointersearch_change_compare_type(&m_Search, type);
  }

  cl_error setTarget(const QString& target) override
  {
    if (target.isEmpty())
      return cl_pointersearch_change_target(&m_Search, NULL);
    switch (m_Search.params.value_type)
    {
    case CL_MEMTYPE_FLOAT:
    case CL_MEMTYPE_DOUBLE:
      return cl_pointersearch_change_target_float(&m_Search, target.toDouble());
    default:
      return cl_pointersearch_change_target_int(&m_Search, (cl_addr_t)target.toLongLong());
    }
  }

  cl_error setValueType(const cl_value_type type) override
  {
    return cl_pointersearch_change_value_type(&m_Search, type);
  }

public slots:
  void onClickResultAddMemoryNote();
  void onResultClick(QTableWidgetItem *item) override;
  void onResultDoubleClick(void) override;
  void onResultEdited(QTableWidgetItem *item) override;
  void onResultRightClick(const QPoint&) override;
  void onResultSelectionChanged(void) override {}

signals:
  void addressChanged(cl_addr_t address);
  void requestAddMemoryNote(cl_memnote_t note);
  void requestRemove(uint32_t index);

private:
  unsigned m_ColAddress;
  unsigned m_ColValuePrev;
  unsigned m_ColValueCurr;
  cl_pointersearch_t m_Search;
};

#endif
