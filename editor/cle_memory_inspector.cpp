#ifndef CLE_MEMORY_INSPECTOR_CPP
#define CLE_MEMORY_INSPECTOR_CPP

#include <QMessageBox>

#include "cle_memory_inspector.h"
#include "cle_common.h"
#include "cle_result_table_normal.h"
#include "cle_result_table_pointer.h"

CleMemoryInspector::CleMemoryInspector()
{
   QGridLayout *Layout = new QGridLayout;

   /* Initialize basic elements */
   m_NewButton = new QPushButton(tr("&New Search"));
   connect(m_NewButton, SIGNAL(clicked()), 
      this, SLOT(onClickNew()));

   m_SearchButton = new QPushButton(tr("&Search"));
   m_SearchButton->setAutoDefault(true);
   connect(m_SearchButton, SIGNAL(clicked()), 
      this, SLOT(onClickSearch()));

   /* Initialize dropdown boxes */
   m_CompareDropdown = new QComboBox();
   m_CompareDropdown->addItem(tr("are equal to..."),      CLE_CMPTYPE_EQUAL);
   m_CompareDropdown->addItem(tr("are greater than..."),  CLE_CMPTYPE_GREATER);
   m_CompareDropdown->addItem(tr("are less than..."),     CLE_CMPTYPE_LESS);
   m_CompareDropdown->addItem(tr("are not equal to..."),  CLE_CMPTYPE_NOT_EQUAL);
   m_CompareDropdown->addItem(tr("have increased by..."), CLE_CMPTYPE_INCREASED);
   m_CompareDropdown->addItem(tr("have decreased by..."), CLE_CMPTYPE_DECREASED);
   m_CompareDropdown->addItem(tr("are above address..."), CLE_CMPTYPE_ABOVE);
   m_CompareDropdown->addItem(tr("are below address..."), CLE_CMPTYPE_BELOW);
   connect(m_CompareDropdown, SIGNAL(activated(int)), 
      this, SLOT(onChangeCompareType()));

   m_SizeDropdown = new QComboBox();
   m_SizeDropdown->addItem(tr("1-byte values"), CL_MEMTYPE_8BIT);
   m_SizeDropdown->addItem(tr("2-byte values"), CL_MEMTYPE_16BIT);
   m_SizeDropdown->addItem(tr("4-byte values"), CL_MEMTYPE_32BIT);
   m_SizeDropdown->addItem(tr("float values"),  CL_MEMTYPE_FLOAT);
   connect(m_SizeDropdown, SIGNAL(activated(int)), 
      this, SLOT(onChangeSizeType()));

   /* Initialize text entry box for comparison value */
   m_TextEntry = new QLineEdit();
   connect(m_TextEntry, SIGNAL(returnPressed()), m_SearchButton, SIGNAL(clicked()));

   /* Initialize tab view for switching between searches */
   m_Tabs = new QTabBar();
   m_Tabs->setContextMenuPolicy(Qt::CustomContextMenu);
   m_Tabs->setExpanding(false);
   m_Tabs->addTab(tr("Search 1"));
   m_Tabs->addTab(tr("+"));
   connect(m_Tabs, SIGNAL(currentChanged(int)), 
      this, SLOT(onChangeTab()));
   connect(m_Tabs, SIGNAL(customContextMenuRequested(const QPoint&)), 
      this, SLOT(onRightClickTabs(const QPoint&)));

   /* Initialize hex value view widget */
   m_HexWidget = new CleHexWidget(this, 1);
   m_BufferPrevious = (uint8_t*)malloc(256);
   m_BufferCurrent  = (uint8_t*)malloc(256);
   connect(m_HexWidget, SIGNAL(offsetEdited(int32_t)), 
      this, SLOT(onHexWidgetOffsetEdited(int32_t)));
   connect(m_HexWidget, SIGNAL(onRightClick(uint32_t)),
      this, SLOT(onHexWidgetRightClick(uint32_t)));
   connect(m_HexWidget, SIGNAL(valueEdited(uint32_t, uint8_t)), 
      this, SLOT(onHexWidgetValueEdited(uint32_t, uint8_t)));
   m_HexWidget->setByteSwapEnabled(memory.endianness);

   /* Initialize timer for updating search rows */
   m_UpdateTimer = new QTimer(this);
   connect(m_UpdateTimer, SIGNAL(timeout()), this, SLOT(update()));
   m_UpdateTimer->start(100);

   m_Searches[0] = new CleResultTableNormal(this);

   /* Initialize window layout */
   Layout->addWidget(m_Tabs,            0, 0, 1, 2);
   Layout->addWidget(m_SizeDropdown,    1, 0);
   Layout->addWidget(m_NewButton,       1, 1);
   Layout->addWidget(m_CompareDropdown, 2, 0);
   Layout->addWidget(m_TextEntry,       2, 1);
   Layout->addWidget(m_SearchButton,    3, 0, 1, 2);
   Layout->addWidget(m_Searches[0]->getTable(), 4, 0, 2, 2);
   Layout->addWidget(m_HexWidget,       6, 0, 2, 2);
   setLayout(Layout);
   setWindowTitle(tr("Live Editor"));

   memset(m_Searches, 0, sizeof(m_Searches));

   /* Initialize other variables */
   m_AddressOffset = 0;
   m_ClickedResult = -1;
   m_CurrentEditedRow = -1;
   m_CurrentTab = 0;
   m_TabCount = 1;
   m_MemoryNoteSubmit = NULL;

   onChangeCompareType();
}

uint8_t CleMemoryInspector::getCurrentCompareType(void)
{
   return m_CompareDropdown->itemData(m_CompareDropdown->currentIndex()).toUInt();
}

uint8_t CleMemoryInspector::getCurrentSizeType(void)
{
   return m_SizeDropdown->itemData(m_SizeDropdown->currentIndex()).toUInt();
}

void CleMemoryInspector::onChangeCompareType()
{
   switch (m_CompareDropdown->itemData(m_CompareDropdown->currentIndex()).toUInt())
   {
   case CLE_CMPTYPE_EQUAL:
   case CLE_CMPTYPE_GREATER:
   case CLE_CMPTYPE_LESS:
   case CLE_CMPTYPE_NOT_EQUAL:
      m_TextEntry->setPlaceholderText(tr("previous value"));
      break;
   case CLE_CMPTYPE_INCREASED:
   case CLE_CMPTYPE_DECREASED:
      m_TextEntry->setPlaceholderText(tr("any amount"));
      break;
   case CLE_CMPTYPE_ABOVE:
   case CLE_CMPTYPE_BELOW:
   default:
      m_TextEntry->setPlaceholderText("");
   }
}

void CleMemoryInspector::onChangeSizeType()
{
   m_HexWidget->setSize(cl_sizeof_memtype(getCurrentSizeType()));
}

void CleMemoryInspector::onChangeTab()
{
   m_CurrentTab = m_Tabs->currentIndex();
   if (m_Tabs->currentIndex() > m_TabCount - 1)
   {
      onClickNew();
      m_Tabs->setTabText(m_Tabs->currentIndex(), tr("New Search"));
      m_Tabs->addTab(tr("+"));
      m_TabCount++;
      m_CurrentSearch = m_Searches[m_CurrentTab];
   }
   m_CurrentSearch->rebuild();
}

void CleMemoryInspector::onClickNew()
{
   if (!memory.bank_count)
      return;
   else
   {
      delete m_CurrentSearch;
      m_CurrentSearch = new CleResultTableNormal(this);
   }
}

void CleMemoryInspector::onClickSearch()
{
   if (memory.bank_count == 0)
      return;
   else
   {
      if (!m_CurrentSearch->isInitted())
         onClickNew();
      if (!m_CurrentSearch->step(m_TextEntry->text(), getCurrentCompareType(), getCurrentSizeType()))
         m_TextEntry->setText("");
   }
}

void CleMemoryInspector::onHexWidgetOffsetEdited(int32_t delta)
{
   /* Round down to nearest row */
   uint32_t new_address = m_AddressOffset + (delta & ~0xF);

   /* Don't underflow, don't scroll into invalid data */
   if ((new_address > m_AddressOffset && delta < 0) || 
       new_address + 256 > memory.banks[0].size)
      return;
   else
   {
      m_AddressOffset = new_address;
      memcpy(m_BufferPrevious, &memory.banks[0].data[new_address], 256);
      m_HexWidget->setOffset(new_address);
   }
}

void CleMemoryInspector::onHexWidgetValueEdited(uint32_t address, uint8_t value)
{
   cl_write_memory(NULL, address, 1, &value);
}

void CleMemoryInspector::onResultClicked()
{
   m_AddressOffset = getClickedResultAddress() & ~0xF;
   memcpy(m_BufferPrevious, &memory.banks[0].data[m_AddressOffset], 256);
   m_HexWidget->setOffset(m_AddressOffset);
}

void CleMemoryInspector::onResultDoubleClicked()
{
   /* TODO "Current" value position might change */
   if (m_ResultTable->currentColumn() == 2)
   {
      uint32_t i;

      /* We gray out the other entries because they won't update while
         we're editing. */
      for (i = 0; i < m_ResultTable->rowCount(); i++)
         m_ResultTable->item(i, 2)->setTextColor(Qt::gray);
      m_CurrentEditedRow = m_ResultTable->currentRow();
   }
}

void CleMemoryInspector::onResultEdited(QTableWidgetItem *result)
{
   if (result->row() == m_CurrentEditedRow && result->column() == 2)
   {
      if (result->isSelected())
      {
         uint32_t  address;
         QString   new_value_text;
         bool      ok = true;
         uint8_t   size;
         void     *value;

         address = getClickedResultAddress();
         size = cl_sizeof_memtype(getCurrentSizeType());

         /* TODO "Current" value position might change */
         new_value_text = m_ResultTable->item(m_CurrentEditedRow, 2)->text();

         if (getCurrentSizeType() == CL_MEMTYPE_FLOAT)
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

void CleMemoryInspector::onResultSelectionChanged()
{
   m_CurrentEditedRow = -1;
}

void CleMemoryInspector::onClickTabRename()
{
   QString text = QInputDialog::getText(this, tr("Rename"), tr("Search tab name:"));

   if (m_ClickedTab >= 0 && m_ClickedTab < m_TabCount && !text.isEmpty())
      m_Tabs->setTabText(m_ClickedTab, text);
}

void CleMemoryInspector::onClickResultAddMemoryNote(void)
{
   if (m_ClickedResult < 0)
      return;
   else if (!session.game_id)
   {
      QMessageBox::warning(this, "Live Editor", 
         tr("The currently played game was not recognized by the server, so you cannot submit memory notes.")
      );
   }
   else
   {
      cl_memnote_t new_note;

      new_note.address = getClickedResultAddress();
      new_note.type = getCurrentSizeType();
      /* TODO */
      new_note.pointer_offsets = NULL;
      new_note.pointer_passes = 0;

      m_MemoryNoteSubmit = new CleMemoryNoteSubmit(new_note);
      m_ClickedResult = -1;

      m_MemoryNoteSubmit->show();
   }
}

/* TODOTODAY: The result table needs to pass an address to here
void CleMemoryInspector::onClickResultPointerSearch(void)
{
   if (m_ClickedResult < 0)
      return;
   else
   {
      m_Searches[m_TabCount] = new CleResultTablePointer;
      cl_pointersearch_init(
         m_Searches[m_TabCount].getSearchData(),
         getClickedResultAddress(),
         cl_sizeof_memtype(getCurrentSizeType()),
         1,
         0x100000,
         10000);

      m_Tabs->setCurrentIndex(m_TabCount);
      m_Tabs->setTabText(m_Tabs->currentIndex(), tr("Pointers"));
      m_Tabs->setTabTextColor(m_Tabs->currentIndex(), Qt::yellow);

      m_CurrentSearch->rebuildRows();

      m_ClickedResult = -1;
   }
}
*/

/*
void CleMemoryInspector::onClickResultRemove(void)
{
   if (m_ClickedResult < 0)
      return;
   else
   {
      cl_search_remove(&m_Searches[m_CurrentTab], getClickedResultAddress());
      rebuildRows();
      m_ClickedResult = -1;
   }
}
*/

void CleMemoryInspector::onHexWidgetRightClick(uint32_t address)
{
   QMenu menu;
   QAction *action_add = menu.addAction(tr("&Add memory note..."));
   QAction *action_ptr = menu.addAction(tr("Search for &pointers..."));

   //connect(action_add, SIGNAL(triggered()), this, 
   //   SLOT(onClickResultAddMemoryNote()));
   //connect(action_ptr, SIGNAL(triggered()), this, 
   //   SLOT(onClickResultPointerSearch()));

   menu.exec(QCursor::pos());
}

void CleMemoryInspector::onRightClickResult(const QPoint &pos)
{
   if (pos.isNull())
      return;
   else
   {
      m_ClickedResult = m_ResultTable->rowAt(pos.y());
      if (m_ClickedResult < 0 || m_ClickedResult >= m_ResultTable->rowCount())
         return;
      else
      {
         QMenu menu;
         QAction *action_add    = menu.addAction(tr("&Add memory note..."));
         QAction *action_ptr    = menu.addAction(tr("Search for &pointers..."));
         QAction *action_remove = menu.addAction(tr("&Remove"));

         connect(action_add, SIGNAL(triggered()), this, 
            SLOT(onClickResultAddMemoryNote()));
         connect(action_ptr, SIGNAL(triggered()), this, 
            SLOT(onClickResultPointerSearch()));
         connect(action_remove, SIGNAL(triggered()), this, 
            SLOT(onClickResultRemove()));

         menu.exec(m_ResultTable->mapToGlobal(pos));
      }
   }
}

void CleMemoryInspector::onRightClickTabs(const QPoint &pos)
{
   if (pos.isNull())
      return;
   else
   {
      m_ClickedTab = m_Tabs->tabAt(pos);
      if (m_ClickedTab < 0 || m_ClickedTab >= m_TabCount)
         return;
      else
      {
         QMenu menu;
         QAction *action_rename = menu.addAction(tr("Rename"));

         connect(action_rename, SIGNAL(triggered()), this, 
            SLOT(onClickTabRename()));

         menu.exec(m_Tabs->mapToGlobal(pos));
      }
   }
}

/*
void CleMemoryInspector::rebuildRowsNormal()
{
   cl_search_t *search;
   char         temp_string[32];
   uint8_t      memtype, size;
   uint32_t     current_row, matches, temp_value, i, j;

   search = &m_Searches[m_CurrentTab];

   /* (De)allocate rows 
   if (search->matches > CLE_SEARCH_MAX_ROWS)
      matches = CLE_SEARCH_MAX_ROWS;
   else if (search->matches == 0)
   {
      m_ResultTable->setRowCount(0);
      return;
   }
   else
      matches = search->matches;

   current_row = 0;
   memtype = getCurrentSizeType();
   size = cl_sizeof_memtype(getCurrentSizeType());

   for (i = 0; i < memory.bank_count; i++)
   {
      if (!m_Searches[m_CurrentTab].searchbanks[i].any_valid)
         continue;

      for (j = 0; j < memory.banks[i].size; j += size)
      {
         /* This value was filtered out 
         if (!m_Searches[m_CurrentTab].searchbanks[i].valid[j])
            continue;

         /* This value is still valid; add a new row 
         m_ResultTable->insertRow(current_row);
         /* Address 
         snprintf(temp_string, 256, "%08X", j + memory.banks[i].start);
         m_ResultTable->setItem(current_row, 0, new QTableWidgetItem(QString(temp_string)));
         /* Previous value 
         cl_read_search(&temp_value, &m_Searches[m_CurrentTab], &m_Searches[m_CurrentTab].searchbanks[i], j, size);
         valueToString(temp_string, sizeof(temp_string), temp_value, memtype);
         m_ResultTable->setItem(current_row, 1, new QTableWidgetItem(QString(temp_string)));
         /* Current value 
         cl_read_memory(&temp_value, &memory.banks[i], j, size);
         valueToString(temp_string, sizeof(temp_string), temp_value, memtype);
         m_ResultTable->setItem(current_row, 2, new QTableWidgetItem(QString(temp_string)));
         current_row++;

         /* No need to continue 
         if (current_row == matches)
         {
            m_ResultTable->setRowCount(matches);
            return;
         }
      }
   }
}
*/

/*
void CleMemoryInspector::rebuildRowsPointer()
{
   cl_pointersearch_t *search;
   char     temp_string[32];
   uint8_t  memtype;
   uint32_t current_row, matches, temp_value, i, j;

   search  = &m_PointerSearch[m_CurrentTab];
   matches = search->result_count;

   m_ResultTable->setColumnCount(3 + search->passes);
   memtype = getCurrentSizeType();

   for (i = 0; i < matches; i++)
   {
      m_ResultTable->insertRow(i);

      snprintf(temp_string, 256, "%08X", search->results[i].address_initial);
      m_ResultTable->setItem(i, 0, new QTableWidgetItem(QString(temp_string)));

      snprintf(temp_string, 256, "%02X", search->results[i].offsets[0]);
      m_ResultTable->setItem(i, 1, new QTableWidgetItem(QString(temp_string)));

      valueToString(temp_string, sizeof(temp_string), search->results[i].value_previous, memtype);
      m_ResultTable->setItem(i, 2, new QTableWidgetItem(QString(temp_string)));

      valueToString(temp_string, sizeof(temp_string), search->results[i].value_current, memtype);
      m_ResultTable->setItem(i, 3, new QTableWidgetItem(QString(temp_string)));
   }
   m_ResultTable->setRowCount(matches);
}
*/

void CleMemoryInspector::update()
{
   m_CurrentSearch->run(getCurrentSizeType(), cl_sizeof_memtype(getCurrentSizeType()));

   if (m_AddressOffset > memory.banks[0].start + memory.banks[0].size)
      return;

   memcpy(m_BufferCurrent, &memory.banks[0].data[m_AddressOffset], 256);
   m_HexWidget->refresh(m_BufferCurrent, m_BufferPrevious);
   memcpy(m_BufferPrevious, m_BufferCurrent, 256);
}

#endif
