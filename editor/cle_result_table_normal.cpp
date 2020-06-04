#ifndef CLE_RESULT_TABLE_NORMAL_CPP
#define CLE_RESULT_TABLE_NORMAL_CPP

#include <QScrollBar>
#include <QStringList>

#include "cle_result_table_normal.h"
#include "cle_common.h"

#define COL_ADDRESS        0
#define COL_PREVIOUS_VALUE 1
#define COL_CURRENT_VALUE  2

CleResultTableNormal::CleResultTableNormal()
{
   CleResultTable::init();

   /* Normal-specific table styling */
   m_Table->setColumnCount(3);

   /* Initialize result table column headers */
   QStringList TableHeader;
   TableHeader << tr("Address") << tr("Previous") << tr("Current");
   m_Table->setHorizontalHeaderLabels(TableHeader);

   cl_search_init(&m_Search);
}

CleResultTableNormal::~CleResultTableNormal()
{
   cl_search_free(&m_Search);
}

uint32_t CleResultTableNormal::getClickedResultAddress()
{
   if (m_ClickedResult < 0)
      return 0;
   else
      return m_Table->item(m_ClickedResult, 0)->text().split(" ")[0].toULong(NULL, 16);
}

void *CleResultTableNormal::getSearchData()
{
   return (void*)(&m_Search);
}

void CleResultTableNormal::onResultClick()
{
   emit addressChanged(getClickedResultAddress() & ~0xF);
}

void CleResultTableNormal::onResultDoubleClick()
{
   if (m_Table->currentColumn() == COL_CURRENT_VALUE)
   {
      uint32_t i;

      /* We gray out the other entries because they won't update while
         we're editing. */
      for (i = 0; i < m_Table->rowCount(); i++)
         m_Table->item(i, COL_CURRENT_VALUE)->setTextColor(Qt::gray);
      m_CurrentEditedRow = m_Table->currentRow();
   }
}

void CleResultTableNormal::onResultEdited(QTableWidgetItem *result)
{
   if (result->row() == m_CurrentEditedRow && result->column() == COL_CURRENT_VALUE)
   {
      if (result->isSelected())
      {
         uint32_t  address;
         QString   new_value_text;
         bool      ok = true;
         uint8_t   size;
         void     *value;

         address = getClickedResultAddress();
         size = cl_sizeof_memtype(m_Search.size);
         new_value_text = m_Table->item(m_CurrentEditedRow, COL_CURRENT_VALUE)->text();

         if (m_Search.size == CL_MEMTYPE_FLOAT)
            value = new float(new_value_text.toFloat(&ok));
         else
            value = new uint32_t(stringToValue(new_value_text, &ok));

         if (ok)
            cl_write_memory(NULL, address, size, value);
         free(value);
      }
      m_CurrentEditedRow = -1;
   }
}

void CleResultTableNormal::onResultSelectionChanged()
{
   m_CurrentEditedRow = -1;
} 

void CleResultTableNormal::rebuild()
{
   char         temp_string[32];
   uint8_t      memtype, size;
   uint32_t     current_row, matches, temp_value, i, j;

   /* (De)allocate rows */
   matches = m_Search.matches;
   if (matches > CLE_SEARCH_MAX_ROWS)
      matches = CLE_SEARCH_MAX_ROWS;
   else if (matches == 0)
   {
      m_Table->setRowCount(0);
      return;
   }

   current_row = 0;
   memtype = 15;//getCurrentSizeType();
   size = 1;//cl_sizeof_memtype(getCurrentSizeType());

   for (i = 0; i < memory.bank_count; i++)
   {
      if (!m_Search.searchbanks[i].any_valid)
         continue;

      for (j = 0; j < memory.banks[i].size; j += size)
      {
         /* This value was filtered out */
         if (!m_Search.searchbanks[i].valid[j])
            continue;

         /* This value is still valid; add a new row */
         m_Table->insertRow(current_row);

         /* Address */
         snprintf(temp_string, 256, "%08X", j + memory.banks[i].start);
         m_Table->setItem(current_row, 0, new QTableWidgetItem(QString(temp_string)));
         /* Previous value */
         cl_read_search(&temp_value, &m_Search, &m_Search.searchbanks[i], j, size);
         valueToString(temp_string, sizeof(temp_string), temp_value, memtype);
         m_Table->setItem(current_row, 1, new QTableWidgetItem(QString(temp_string)));
         /* Current value */
         cl_read_memory(&temp_value, &memory.banks[i], j, size);
         valueToString(temp_string, sizeof(temp_string), temp_value, memtype);
         m_Table->setItem(current_row, 2, new QTableWidgetItem(QString(temp_string)));
         current_row++;

         /* No need to continue */
         if (current_row == matches)
         {
            m_Table->setRowCount(matches);
            return;
         }
      }
   }
}

void CleResultTableNormal::reset()
{
   cl_search_free(&m_Search);
   cl_search_init(&m_Search);
   m_Table->setRowCount(0);
}

void CleResultTableNormal::run(uint8_t type, uint8_t size)
{
   QTableWidgetItem *item;
   char     temp_string[32];
   uint32_t address, curr_value, prev_value, i, j;

   for (i = 0; i < m_Table->rowCount(); i++)
   {
      item = m_Table->item(i, 0);
      if (!item)
         return;

      /* Don't visually update search results that are out of view */
      if (i < m_Table->verticalScrollBar()->value())
         continue;
      else if (i > m_Table->verticalScrollBar()->value() + m_Table->size().height() / 16)
         break;

      /* Kind of gross, but should save some memory 
         Only a few results should be redrawn at any time anyway */
      address = item->text().split(" ")[0].toULong(NULL, 16);

      if (!cl_read_memory(&curr_value, NULL, address, size) ||
          !cl_read_search(&prev_value, &m_Search, NULL, address, size))
         break;

      /* Update previous value column */
      item = m_Table->item(i, 1);
      valueToString(temp_string, sizeof(temp_string), prev_value, type);
      item->setText(temp_string);

      /* Update current value column */
      item = m_Table->item(i, 2);
      if (m_CurrentEditedRow != i)
      {
         valueToString(temp_string, sizeof(temp_string), curr_value, type);
         item->setText(temp_string);
      }

      /* Display changed values in red */
      if (prev_value != curr_value)
         item->setTextColor(Qt::red);
      else
         item->setTextColor(Qt::white);
   }
}

bool CleResultTableNormal::step(const QString& text, uint8_t compare_type, uint8_t mem_type)
{
   void    *compare_value;
   uint8_t  compare_size;
   bool     no_input, is_float;
   bool     ok = true;

   compare_size = cl_sizeof_memtype(mem_type);
   no_input     = text.isEmpty();
   is_float     = mem_type == CL_MEMTYPE_FLOAT ? true : false;

   if (is_float)
      compare_value = new float(text.toFloat(&ok));
   else
      compare_value = new uint32_t(stringToValue(text, &ok));

   /* Run the C code for doing the actual search */
   if (ok || no_input)
      cl_search_step
      (
         &m_Search,
         no_input ? NULL : compare_value, 
         compare_size, 
         compare_type,
         is_float
      );
   else if (text.front() == '"' && text.back() == '"')
      cl_search_ascii
      (
         &m_Search, 
         text.mid(1, text.length() - 2).toStdString().c_str(), 
         text.length() - 2
      );
   else
      return false;
   free(compare_value);
   rebuild();

   return true;
}

/*
case CLE_SEARCHTYPE_POINTER:
            cl_pointersearch_step(
               &m_PointerSearch[m_CurrentTab],
               no_input ? NULL : (uint32_t*)compare_value,
               compare_size,
               compare_type);
            break;
            */

#endif
