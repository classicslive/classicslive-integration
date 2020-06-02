#ifndef CLE_RESULT_TABLE_NORMAL_H
#define CLE_RESULT_TABLE_NORMAL_H

#include <QWidget>

extern "C"
{
   #include "../cl_memory.h"
   #include "../cl_search.h"
}

#include "cle_result_table.h"

/* TODO: Move this to a config option? */
#define CLE_SEARCH_MAX_ROWS 1000

class CleResultTableNormal : public CleResultTable
{
   Q_OBJECT

public:
   CleResultTableNormal(QWidget *parent);
   ~CleResultTableNormal();

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
   cl_search_t m_Search;
};

#endif
