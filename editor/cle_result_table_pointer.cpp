#ifndef CLE_RESULT_TABLE_POINTER_CPP
#define CLE_RESULT_TABLE_POINTER_CPP

#include <QScrollBar>

#include "cle_result_table_pointer.h"
#include "cle_common.h"

uint32_t CleResultTablePointer::getClickedResultAddress()
{
   if (m_ClickedResult < 0)
      return 0;
   else
      return m_Search.results[m_ClickedResult].address_final;
}

void* CleResultTablePointer::getSearchData()
{
   return (void*)(&m_Search);
}

void CleResultTablePointer::rebuild()
{
   char     temp_string[32];
   uint8_t  memtype;
   uint32_t current_row, temp_value, i, j;

   m_Table->setColumnCount(3 + m_Search.passes);

   for (i = 0; i < m_Search.result_count; i++)
   {
      m_Table->insertRow(i);

      snprintf(temp_string, 256, "%08X", m_Search.results[i].address_initial);
      m_Table->setItem(i, 0, new QTableWidgetItem(QString(temp_string)));

      snprintf(temp_string, 256, "%02X", m_Search.results[i].offsets[0]);
      m_Table->setItem(i, 1, new QTableWidgetItem(QString(temp_string)));

      valueToString(temp_string, sizeof(temp_string), m_Search.results[i].value_previous, m_Search.size);
      m_Table->setItem(i, 2, new QTableWidgetItem(QString(temp_string)));

      valueToString(temp_string, sizeof(temp_string), m_Search.results[i].value_current, m_Search.size);
      m_Table->setItem(i, 3, new QTableWidgetItem(QString(temp_string)));
   }
   m_Table->setRowCount(m_Search.result_count);
}

void CleResultTablePointer::run(uint8_t type, uint8_t size)
{
   QTableWidgetItem *item;
   char     temp_string[32];
   uint32_t address, value_curr, value_prev, i, j;

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
      valueToString(temp_string, sizeof(temp_string), value_prev, type);
      item->setText(temp_string);

      /* Current value column */
      item = m_Table->item(i, j + 1);
      if (m_CurrentEditedRow != i)
      {
         valueToString(temp_string, sizeof(temp_string), value_curr, type);
         item->setText(temp_string);
      }

      /* Display changed values in red */
      if (value_prev != value_curr)
         item->setTextColor(Qt::red);
      else
         item->setTextColor(Qt::white);
   }
   m_Table->setRowCount(m_Search.result_count);
}

bool CleResultTablePointer::step(const QString& text, uint8_t compare_type, uint8_t mem_type)
{
   return false; //todo
}

#endif
