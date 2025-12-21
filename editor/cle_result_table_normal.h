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
