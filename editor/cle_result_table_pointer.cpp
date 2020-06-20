#ifndef CLE_RESULT_TABLE_POINTER_CPP
#define CLE_RESULT_TABLE_POINTER_CPP

#include <QScrollBar>

#include "cle_result_table_pointer.h"
#include "cle_common.h"

CleResultTablePointer::CleResultTablePointer(uint32_t address, uint8_t size, 
   uint8_t passes, uint32_t range, uint32_t max_results)
{
   uint8_t i;

   CleResultTable::init();

   /* Pointer-specific table styling */
   m_Table->setColumnCount(3 + passes);

   /* Initialize result table column headers */
   QStringList TableHeader;
   TableHeader << tr("Initial");
   for (i = 0; i < passes; i++)
      TableHeader << tr("Offset ") + i;
   TableHeader << tr("Previous") << tr("Current");
   m_Table->setHorizontalHeaderLabels(TableHeader);

   cl_pointersearch_init(&m_Search, address, size, passes, range, max_results);
   rebuild();
}

CleResultTablePointer::~CleResultTablePointer()
{
   cl_pointersearch_free(&m_Search);
}

uint32_t CleResultTablePointer::getClickedResultAddress()
{
   /* TODO: Maybe make an option to seek to initial address instead of final */
   return m_Search.results[m_Table->currentRow()].address_final;
}

void* CleResultTablePointer::getSearchData()
{
   return (void*)(&m_Search);
}

void CleResultTablePointer::rebuild()
{
   char     temp_string[32];
   uint8_t  size;
   uint32_t current_row, temp_value, i, j;

   size = m_Search.params.value_type;
   m_Table->setColumnCount(3 + m_Search.passes);
   m_Table->setRowCount(m_Search.result_count);

   for (i = 0; i < m_Search.result_count; i++)
   {
      m_Table->insertRow(i);

      snprintf(temp_string, 256, "%08X", m_Search.results[i].address_initial);
      m_Table->setItem(i, 0, new QTableWidgetItem(QString(temp_string)));

      snprintf(temp_string, 256, "%02X", m_Search.results[i].offsets[0]);
      m_Table->setItem(i, 1, new QTableWidgetItem(QString(temp_string)));

      valueToString(temp_string, sizeof(temp_string), m_Search.results[i].value_previous, size);
      m_Table->setItem(i, 2, new QTableWidgetItem(QString(temp_string)));

      valueToString(temp_string, sizeof(temp_string), m_Search.results[i].value_current, size);
      m_Table->setItem(i, 3, new QTableWidgetItem(QString(temp_string)));
   }
   m_Table->setRowCount(m_Search.result_count);
}

void CleResultTablePointer::reset(uint8_t value_type)
{
   cl_pointersearch_free(&m_Search);
}

void CleResultTablePointer::run()
{
   QTableWidgetItem *item;
   char     temp_string[32];
   uint8_t  val_type;
   uint32_t address, value_curr, value_prev, i, j;

   val_type = m_Search.params.value_type;

   /* The C code updates all of the pointer results */
   cl_pointersearch_update(&m_Search);

   for (i = 0; i < m_Search.result_count; i++)
   {
      item = m_Table->item(i, 0);
      if (!item)
         return;

      value_curr = m_Search.results[i].value_current;
      value_prev = m_Search.results[i].value_previous;

      /* Don't visually update search results that are out of view */
      if (i < m_Table->verticalScrollBar()->value())
         continue;
      else if (i > m_Table->verticalScrollBar()->value() 
          + m_Table->size().height() / 16)
         break;

      j = m_Search.passes + 1;

      /* Update previous value column */
      item = m_Table->item(i, j);
      valueToString(temp_string, sizeof(temp_string), value_prev, val_type);
      item->setText(temp_string);

      /* Current value column */
      item = m_Table->item(i, j + 1);
      if (m_CurrentEditedRow != i)
      {
         valueToString(temp_string, sizeof(temp_string), value_curr, val_type);
         item->setText(temp_string);
      }

      /* Display changed values in red */
      if (value_prev != value_curr)
         item->setTextColor(Qt::red);
      else
         item->setTextColor(Qt::white);
   }
}

bool CleResultTablePointer::step(const QString& text)
{
   void *compare_value;
   bool  no_input = text.isEmpty();
   bool  ok = true;

   if (m_Search.params.value_type == CL_MEMTYPE_FLOAT)
      compare_value = new float(text.toFloat(&ok));
   else
      compare_value = new uint32_t(stringToValue(text, &ok));

   /* Run the C code for doing the actual search */
   if (ok || no_input)
      cl_pointersearch_step
      (
         &m_Search,
         no_input ? NULL : compare_value
      );
   else
      return false;

   free(compare_value);
   rebuild();

   return true;
}

#endif
