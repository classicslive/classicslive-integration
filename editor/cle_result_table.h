#ifndef CLE_RESULT_TABLE_H
#define CLE_RESULT_TABLE_H

#include <QTableWidget>
#include <QWidget>

extern "C"
{
  #include "../cl_search_new.h"
}

class CleResultTable : public QWidget
{
  Q_OBJECT

public:
  QTableWidget *table(void);

  /**
   * Return an address representing the last clicked row.
   */
  virtual cl_addr_t getClickedResultAddress(void) = 0;

  /**
   * Return a pointer to the relevant search data.
   * (Dereference into cl_search_t, cl_pointersearch_t, etc.)
   */
  virtual void *searchData(void) = 0;

  virtual int isInitted(void) { return 0; }

  /**
   * Recreate all rows to adapt to changes in the search data.
   * (Use when a search is reset, stepped through, etc.)
   */
  virtual cl_error rebuild(void) = 0;

  virtual cl_error reset(void) = 0;
   
  /**
   * Redraw the currently visible table rows with new information.
   * Should be called every frame or another small interval.
   */
  virtual cl_error run(void) = 0;

  virtual cl_error step(void) = 0;

  virtual cl_compare_type compareType(void) { return CL_COMPARE_INVALID; }
  virtual cl_value_type valueType(void) { return CL_MEMTYPE_NOT_SET; }

  virtual cl_error setCompareType(const cl_compare_type type) = 0;
  virtual cl_error setValueType(const cl_value_type type) = 0;

public slots:
  virtual void onResultClick(QTableWidgetItem *item) = 0;
  virtual void onResultDoubleClick(void) = 0;
  virtual void onResultEdited(QTableWidgetItem *item) = 0;
  virtual void onResultRightClick(const QPoint&) = 0;
  virtual void onResultSelectionChanged(void) = 0;

protected:
  /**
   * The last row an edit dialog was created on, used to write the entered
   * value back to the right spot in memory.
   */
  int m_CurrentEditedRow = -1;
  int m_ClickedResult = -1;
  QTableWidget *m_Table = nullptr;

  /**
   * Set up the table. Should only be called once.
   */
  cl_error init(void);

   /**
    * Writes user input from a result entry into emulated memory.
    * @param address The address to write to.
    * @param params The params of the search type.
    * @param string The string the user entered into the result entry.
    */
  cl_error writeMemory(const cl_addr_t address, const cl_search_parameters_t& params,
    const QString& string);
};

#endif
