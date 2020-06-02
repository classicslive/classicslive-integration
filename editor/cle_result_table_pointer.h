#ifndef CLE_RESULT_TABLE_POINTER_H
#define CLE_RESULT_TABLE_POINTER_H

#include "cle_result_table.h"

class CleResultTablePointer : public CleResultTable
{
    Q_OBJECT

public:
   CleResultTablePointer(QWidget *parent);

   uint32_t getClickedResultAddress() override;
   void* getSearchData() override;
   void rebuild() override;
   void reset() override;
   void run(uint8_t typse, uint8_t size) override;
   bool step(const QString& text, uint8_t compare_type, uint8_t mem_type) override;

signals:
   void requestAddMemoryNote(uint32_t index) override;
   void requestPointerSearch(uint32_t index) override;
   void requestRemove(uint32_t index) override;

private:
   cl_pointersearch_t m_Search;
};

#endif
