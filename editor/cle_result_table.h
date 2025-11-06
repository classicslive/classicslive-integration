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
   virtual cl_addr_t getClickedResultAddress() = 0;

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

   virtual void reset(uint8_t value_type) = 0;
   
   /*
      Redraw the currently visible table rows with new information.
      Should be called every frame or another small interval.
   */
   virtual void run() = 0;

   virtual bool step(const QString& text) = 0;

   virtual uint8_t getCompareType() { return 0; }
   virtual uint8_t getValueType() { return 0; }

   virtual void setCompareType(const uint8_t new_type) = 0;
   virtual void setValueType(const cl_value_type new_type) = 0;

public slots:
   virtual void onResultClick(QTableWidgetItem *item) = 0;
   virtual void onResultDoubleClick(void) = 0;
   virtual void onResultEdited(QTableWidgetItem *item) = 0;
   virtual void onResultRightClick(const QPoint&) = 0;
   virtual void onResultSelectionChanged(void) = 0;

protected:
   /* 
      The last row an edit dialog was created on, used to write the entered
      value back to the right spot in memory.
   */
   int32_t m_CurrentEditedRow;

   int32_t m_ClickedResult;

   QTableWidget *m_Table;

   /*
      Set up the table. Should only be called once.
   */
   void init();

   /**
    * Writes user input from a result entry into emulated memory.
    * @param address The address to write to.
    * @param params The params of the search type.
    * @param string The string the user entered into the result entry.
    **/
   void writeMemory(const cl_addr_t address, const cl_search_params_t& params,
      const QString& string);
};

#endif
