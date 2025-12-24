#ifndef CLE_RESULT_TABLE_POINTER_H
#define CLE_RESULT_TABLE_POINTER_H

#include "cle_result_table.h"

extern "C"
{
  #include "../cl_search.h"
}

class CleResultTablePointer : public CleResultTable
{
    Q_OBJECT

public:
  CleResultTablePointer(QWidget *parent, uint32_t address, uint8_t size,
    uint8_t passes, uint32_t range, uint32_t max_results);
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
  cl_value_type valueType(void) override { return CL_MEMTYPE_NOT_SET; }

  cl_error setCompareType(const cl_compare_type type) override {}
  cl_error setTarget(const QString& target) override {}
  cl_error setValueType(const cl_value_type type) override {}

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
  uint8_t m_ColAddress;
  uint8_t m_ColValuePrev;
  uint8_t m_ColValueCurr;
  cl_pointersearch_t m_Search;
};

#endif
