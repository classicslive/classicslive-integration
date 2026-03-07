#include <QMenu>
#include <QScrollBar>

#include "cle_result_table_pointer.h"
#include "cle_common.h"

CleResultTablePointer::CleResultTablePointer(QWidget *parent, cl_addr_t address,
   cl_value_type value_type, unsigned passes, cl_addr_t range, cl_addr_t max_results)
{
   char offset_str[16];
   unsigned i;

   CleResultTable::init();

   /* Pointer-specific table styling */
   m_Table->setColumnCount(3 + passes);

   /* Initialize result table column headers */
   QStringList TableHeader;
   TableHeader << tr("Initial");
   for (i = 0; i < passes; i++)
   {
      snprintf(offset_str, sizeof(offset_str), "Offset %u", i + 1);
      TableHeader << offset_str;
   }
   TableHeader << tr("Previous") << tr("Current");
   m_Table->setHorizontalHeaderLabels(TableHeader);

   m_ColAddress   = 0;
   m_ColValuePrev = passes + 1;
   m_ColValueCurr = passes + 2;

   /* Qt connections to parent */
   connect(this, SIGNAL(addressChanged(cl_addr_t)),
      parent, SLOT(onAddressChanged(cl_addr_t)));
   connect(this, SIGNAL(requestAddMemoryNote(cl_memnote_t)),
      parent, SLOT(requestAddMemoryNote(cl_memnote_t)));
   connect(this, SIGNAL(requestPointerSearch(cl_addr_t)),
      parent, SLOT(requestPointerSearch(cl_addr_t)));

   if (cl_pointersearch_init(&m_Search, address, value_type, passes, range, max_results) != CL_OK)
   {
      cl_log("Failed to initialize pointer search for address %016llX\n", (unsigned long long)address);
   }
   rebuild();
}

CleResultTablePointer::~CleResultTablePointer()
{
   cl_pointersearch_free(&m_Search);
}

cl_addr_t CleResultTablePointer::getClickedResultAddress()
{
   return m_Search.results[m_Table->currentRow()].address_final;
}

void *CleResultTablePointer::searchData(void)
{
  return &m_Search;
}

void CleResultTablePointer::onClickResultAddMemoryNote()
{
   cl_memnote_t note;

   note.address_initial = m_Search.results[m_ClickedResult].address_initial;
   note.type = m_Search.params.value_type;
   memcpy(note.pointer_offsets, m_Search.results[m_ClickedResult].offsets, sizeof(note.pointer_offsets));
   note.pointer_passes = m_Search.passes;

   emit requestAddMemoryNote(note);
}

void CleResultTablePointer::onResultRightClick(const QPoint& pos)
{
   if (pos.isNull())
      return;
   else
   {
      m_ClickedResult = m_Table->rowAt(pos.y());
      if (m_ClickedResult < 0 || m_ClickedResult >= m_Table->rowCount())
         return;
      else
      {
         QMenu menu;
         QAction *action_add = menu.addAction(tr("&Add memory note..."));

         connect(action_add, SIGNAL(triggered()), this,
            SLOT(onClickResultAddMemoryNote()));

         menu.exec(m_Table->mapToGlobal(pos));
      }
   }
}

/* TODO: Allow seeking to multiple steps */
void CleResultTablePointer::onResultClick(QTableWidgetItem *item)
{
   if (!item)
      return;
   else
   {
      if (item->column() == 0)
         emit addressChanged(m_Search.results[item->row()].address_initial);
      else
         emit addressChanged(m_Search.results[item->row()].address_final);
   }
}

void CleResultTablePointer::onResultDoubleClick()
{
   if (m_Table->currentColumn() == (int)m_ColValueCurr)
   {
      unsigned i;

      /* We gray out the other entries because they won't update while
         we're editing. */
      for (i = 0; i < (unsigned)m_Table->rowCount(); i++)
         m_Table->item(i, m_ColValueCurr)->setForeground(Qt::gray);
      m_CurrentEditedRow = m_Table->currentRow();
   }
}

void CleResultTablePointer::onResultEdited(QTableWidgetItem *item)
{
   if (item->row() == m_CurrentEditedRow && item->column() == (int)m_ColValueCurr)
   {
      if (item->isSelected())
         writeMemory(m_Search.results[item->row()].address_final, m_Search.params.value_type, item->text());
      m_CurrentEditedRow = -1;
   }
}

cl_error CleResultTablePointer::rebuild(void)
{
   char          temp_string[32];
   cl_value_type val_type = m_Search.params.value_type;
   unsigned      i, j;

   m_Table->setRowCount(0);

   for (i = 0; i < m_Search.result_count; i++)
   {
      m_Table->insertRow(i);

      snprintf(temp_string, sizeof(temp_string), "%016llX",
         (unsigned long long)m_Search.results[i].address_initial);
      m_Table->setItem(i, m_ColAddress, new QTableWidgetItem(QString(temp_string)));

      for (j = 0; j < m_Search.passes; j++)
      {
         snprintf(temp_string, sizeof(temp_string), "%llX",
            (unsigned long long)m_Search.results[i].offsets[j]);
         m_Table->setItem(i, j + 1, new QTableWidgetItem(QString(temp_string)));
      }

      valueToString(temp_string, sizeof(temp_string),
         m_Search.results[i].value_previous.raw, val_type);
      m_Table->setItem(i, m_ColValuePrev, new QTableWidgetItem(QString(temp_string)));

      valueToString(temp_string, sizeof(temp_string),
         m_Search.results[i].value_current.raw, val_type);
      m_Table->setItem(i, m_ColValueCurr, new QTableWidgetItem(QString(temp_string)));
   }

   return CL_OK;
}

cl_error CleResultTablePointer::reset(void)
{
   cl_pointersearch_free(&m_Search);
   m_Table->setRowCount(0);

   return CL_OK;
}

cl_error CleResultTablePointer::run(void)
{
   QTableWidgetItem *item;
   char          temp_string[32];
   cl_value_type val_type  = m_Search.params.value_type;
   unsigned      val_size  = m_Search.params.value_size;
   unsigned      i;

   cl_pointersearch_update(&m_Search);

   for (i = 0; i < m_Search.result_count; i++)
   {
      item = m_Table->item(i, 0);
      if (!item)
         return CL_OK;

      /* Don't visually update search results that are out of view */
      if ((int)i < m_Table->verticalScrollBar()->value())
         continue;
      else if ((int)i > m_Table->verticalScrollBar()->value() + m_Table->height() / 16)
         break;

      /* Update previous value column */
      item = m_Table->item(i, m_ColValuePrev);
      if (item)
      {
         valueToString(temp_string, sizeof(temp_string),
            m_Search.results[i].value_previous.raw, val_type);
         item->setText(temp_string);
      }

      /* Current value column */
      if (m_CurrentEditedRow < 0)
      {
         item = m_Table->item(i, m_ColValueCurr);
         if (item)
         {
            valueToString(temp_string, sizeof(temp_string),
               m_Search.results[i].value_current.raw, val_type);
            item->setText(temp_string);

            bool changed = memcmp(m_Search.results[i].value_current.raw,
               m_Search.results[i].value_previous.raw, val_size) != 0;
            item->setForeground(changed ? Qt::red : Qt::white);
         }
      }
   }

   return CL_OK;
}

cl_error CleResultTablePointer::step(void)
{
  cl_pointersearch_step(&m_Search);
  rebuild();

  return CL_OK;
}
