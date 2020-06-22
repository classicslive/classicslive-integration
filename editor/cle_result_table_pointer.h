#ifndef CLE_RESULT_TABLE_POINTER_H
#define CLE_RESULT_TABLE_POINTER_H

#include "cle_result_table.h"

class CleResultTablePointer : public CleResultTable
{
    Q_OBJECT

public:
   CleResultTablePointer(uint32_t address, uint8_t size, uint8_t passes, 
      uint32_t range, uint32_t max_results);
   ~CleResultTablePointer();

   uint32_t getClickedResultAddress() override;
   void* getSearchData() override;
   bool isInitted() { return true; }
   void rebuild() override;
   void reset(uint8_t value_type) override;
   void run() override;
   bool step(const QString& text) override;

   uint8_t getCompareType() { return m_Search.params.compare_type; }
   uint8_t getValueType() { return m_Search.params.value_type; }

   void setCompareType(const uint8_t new_type) { m_Search.params.compare_type = new_type; }
   void setValueType(const uint8_t new_type) { m_Search.params.value_type = new_type; m_Search.params.size = cl_sizeof_memtype(new_type); }

public slots:
   void onResultClick(QTableWidgetItem *item) override;
   void onResultDoubleClick() override;
   void onResultEdited(QTableWidgetItem *item) override;

signals:
   void addressChanged(uint32_t address);
   //void requestAddMemoryNote(uint32_t index) override;
   //void requestPointerSearch(uint32_t index) override;
   //void requestRemove(uint32_t index) override;

private:
   uint8_t m_ColAddress;
   uint8_t m_ColValuePrev;
   uint8_t m_ColValueCurr;
   cl_pointersearch_t m_Search;
};

#endif
