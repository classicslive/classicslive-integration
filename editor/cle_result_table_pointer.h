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
   void rebuild() override;
   void reset(uint8_t value_type) override;
   void run() override;
   bool step(const QString& text) override;

   virtual uint8_t getCompareType() { return m_Search.params.compare_type; }
   virtual uint8_t getValueType() { return m_Search.params.value_type; }

   virtual void setCompareType(const uint8_t new_type) { m_Search.params.compare_type = new_type; }
   virtual void setValueType(const uint8_t new_type) { m_Search.params.value_type = new_type; }

signals:
   void requestAddMemoryNote(uint32_t index) override;
   void requestPointerSearch(uint32_t index) override;
   void requestRemove(uint32_t index) override;

private:
   cl_pointersearch_t m_Search;
};

#endif
