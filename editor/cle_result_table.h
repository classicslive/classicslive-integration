#ifndef CLE_RESULT_TABLE_H
#define CLE_RESULT_TABLE_H

#include <stdint.h>

#include <QTableWidget>
#include <QWidget>

extern "C"
{
   #include "../cl_memory.h"
   #include "../cl_search.h"
}

class CleResultTable : public QWidget
{
   Q_OBJECT

public:
   QTableWidget *getTable();

   /*
      Return an address representing the last clicked row.
   */
   virtual uint32_t getClickedResultAddress() = 0;

   /* 
      Return a pointer to the relevant search data.
      (Dereference into cl_search_t, cl_pointersearch_t, etc.)
   */
   virtual void* getSearchData() = 0;

   virtual bool isInitted() { return false; }

   /*
      Recreate all rows to adapt to changes in the search data.
      (Use when a search is reset, stepped through, etc.)
   */
   virtual void rebuild() = 0;

   virtual void reset() = 0;
   
   /*
      Redraw the currently visible table rows with new information.
      Should be called every frame or another small interval.
   */
   virtual void run(uint8_t type, uint8_t size) = 0;

   virtual bool step(const QString& text, uint8_t compare_type, uint8_t mem_type) = 0;

signals:
   virtual void requestAddMemoryNote(uint32_t index) = 0;
   virtual void requestPointerSearch(uint32_t index) = 0;
   virtual void requestRemove(uint32_t index) = 0;

public slots:
   virtual void onResultClick(void){};
   virtual void onResultDoubleClick(void){};
   virtual void onResultEdited(QTableWidgetItem *item){};
   virtual void onResultRightClick(const QPoint&){};
   virtual void onResultSelectionChanged(void){};

protected:
   /* 
      Index of the last row clicked
   */
   int32_t m_ClickedResult;

   /* 
      The last row an edit dialog was created on, used to write the entered
      value back to the right spot in memory.
   */
   int32_t m_CurrentEditedRow;

   QTableWidget *m_Table;

   /*
      Set up the table. Should only be called once.
   */
   void init();
};

#endif
