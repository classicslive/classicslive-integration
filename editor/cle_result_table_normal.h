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
  void* getSearchData() override;
  bool isInitted() override { return true; }
  void rebuild() override;
  void reset(void) override;
  void run() override;
  bool step(const QString& text) override;

  cl_comparison getCompareType(void) override { return m_Search.comparison; }
  cl_value_type getValueType(void) override { return m_Search.value_type; }

  void setCompareType(const cl_comparison new_type) override
  {
    m_Search.comparison = new_type;
  }
  void setValueType(const cl_value_type new_type) override
  {
    m_Search.value_type = new_type;
    m_Search.value_size = cl_sizeof_memtype(new_type);
  }

public slots:
   void onClickResultAddMemoryNote();
   void onClickResultPointerSearch();
   void onClickResultRemove();
   void onResultClick(QTableWidgetItem *item) override;
   void onResultDoubleClick(void) override;
   void onResultEdited(QTableWidgetItem *item) override;
   void onResultRightClick(const QPoint&) override;
   void onResultSelectionChanged(void) override;

signals:
   void addressChanged(cl_addr_t address);
   void requestAddMemoryNote(cl_memnote_t note);
   void requestPointerSearch(cl_addr_t address);
   //void requestRemove(uint32_t index) override;

private:
   cl_search_t m_Search;
};

#endif
