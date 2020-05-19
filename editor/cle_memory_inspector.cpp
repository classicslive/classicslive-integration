#ifndef CLE_MEMORY_INSPECTOR_CPP
#define CLE_MEMORY_INSPECTOR_CPP

#include <QMessageBox>

#include "cle_memory_inspector.h"

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

   /* Initialize memory search result table */
   m_ResultTable = new QTableWidget();
   m_ResultTable->setRowCount(0);
   m_ResultTable->setColumnCount(3);
   m_ResultTable->setContextMenuPolicy(Qt::CustomContextMenu);
   m_ResultTable->setAlternatingRowColors(true);
   m_ResultTable->setShowGrid(false);
   m_ResultTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::Stretch);
   m_ResultTable->verticalHeader()->setVisible(false);
   m_ResultTable->verticalHeader()->setDefaultSectionSize(16);
   connect(m_ResultTable, SIGNAL(itemClicked(QTableWidgetItem*)), 
      this, SLOT(onResultClicked()));
   connect(m_ResultTable, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), 
      this, SLOT(onResultDoubleClicked()));
   connect(m_ResultTable, SIGNAL(itemChanged(QTableWidgetItem*)), 
      this, SLOT(onResultEdited(QTableWidgetItem*)));
   connect(m_ResultTable, SIGNAL(customContextMenuRequested(const QPoint&)), 
      this, SLOT(onRightClickResult(const QPoint&)));
   connect(m_ResultTable, SIGNAL(itemSelectionChanged()), 
      this, SLOT(onResultSelectionChanged()));

   /* Initialize result table column headers */
   QStringList TableHeader;
   TableHeader << tr("Address") << tr("Previous") << tr("Current");
   m_ResultTable->setHorizontalHeaderLabels(TableHeader);

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
   connect(m_HexWidget, SIGNAL(valueEdited(uint32_t, uint8_t)), 
      this, SLOT(onHexWidgetValueEdited(uint32_t, uint8_t)));

   /* Initialize timer for updating search rows */
   m_UpdateTimer = new QTimer(this);
   connect(m_UpdateTimer, SIGNAL(timeout()), this, SLOT(update()));
   m_UpdateTimer->start(100);

   /* Initialize window layout */
   Layout->addWidget(m_Tabs,            0, 0, 1, 2);
   Layout->addWidget(m_SizeDropdown,    1, 0);
   Layout->addWidget(m_NewButton,       1, 1);
   Layout->addWidget(m_CompareDropdown, 2, 0);
   Layout->addWidget(m_TextEntry,       2, 1);
   Layout->addWidget(m_SearchButton,    3, 0, 1, 2);
   Layout->addWidget(m_ResultTable,     4, 0, 2, 2);
   Layout->addWidget(m_HexWidget,       6, 0, 2, 2);
   setLayout(Layout);
   setWindowTitle(tr("Live Editor"));

   memset(m_Searches, 0, sizeof(m_Searches));
   memset(m_PointerSearch, 0, sizeof(m_PointerSearch));
   memset(m_SearchTypes, CLE_SEARCHTYPE_NORMAL, sizeof(m_SearchTypes));

   /* Initialize other variables */
   m_AddressOffset = 0;
   m_ClickedResult = -1;
   m_CurrentEditedRow = -1;
   m_CurrentTab = 0;
   m_TabCount = 1;
   m_MemoryNoteSubmit = NULL;

   onChangeCompareType();
}

inline uint32_t stringToValue(QString string, bool *ok)
{
   if (string.isEmpty())
      return 0;
   else
   {
      uint8_t base = 0;
      bool negative = false;
      char prefix = string.at(0).toLatin1();

      /* Pop one more if entered value is negative */
      if (prefix == '-')
      {
         negative = true;
         string.remove(0, 1);
         prefix = string.at(0).toLatin1();
      }

      /* Return interpretation of entered number */
      switch (tolower(prefix))
      {
      case 'b':
         /* Binary */
         base = 2;
         break;
      case 'o':
         /* Octal */
         base = 8;
         break;
      case 'd':
         /* Decimal */
         base = 10;
         break;
      case 'h':
      case 'x':
         /* Hexidecimal */
         base = 16;
         break;
      case 'c':
      case 's':
         /* ASCII string */
         /* Bad; remove this if we ever add a text search type */ 
         {
            uint8_t length = string.length() < 5 ? string.length() - 1 : 4;
            uint32_t value = 0;
            uint8_t i;

            string.remove(0, 1);
            for (i = 0; i < length; ++i)
               value |= (uint8_t)string.at(i).toLatin1() << ((length - 1) * 8 - i * 8);

            return value;
         }
      }

      if (base)
         string.remove(0, 1);
      else
         base = 10; // TODO: Config option for default base?

      return negative ? 0 - (uint32_t)string.toInt(ok, base) : 
         string.toUInt(ok, base);
   }
}

inline uint32_t CleMemoryInspector::getClickedResultAddress(void)
{
   return m_ResultTable->item(m_ResultTable->currentRow(), 0)->
      text().split(" ")[0].toULong(NULL, 16);
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
   }
   else
      rebuildRows();
}

void CleMemoryInspector::onClickNew()
{
   if (!memory.bank_count)
      return;
   else
   {  
      cl_search_free(&m_Searches[m_CurrentTab]);
      cl_search_init(&m_Searches[m_CurrentTab]);
      cl_search_reset(&m_Searches[m_CurrentTab]);
      m_ResultTable->setRowCount(0);
   }
}

void CleMemoryInspector::onClickSearch()
{
   if (memory.bank_count == 0)
      return;
   else
   {
      void    *compare_value;
      uint8_t  compare_size, compare_type;
      bool     no_input, is_float;
      bool     ok = true;

      if (m_Searches[m_CurrentTab].searchbank_count == 0)
         onClickNew();

      compare_size = cl_sizeof_memtype(getCurrentSizeType());
      compare_type = m_CompareDropdown->itemData(m_CompareDropdown->currentIndex()).toUInt();
      no_input     = m_TextEntry->text().isEmpty();
      is_float     = getCurrentSizeType() == CL_MEMTYPE_FLOAT ? true : false;

      if (is_float)
         compare_value = new float(m_TextEntry->text().toFloat(&ok));
      else
         compare_value = new uint32_t(stringToValue(m_TextEntry->text(), &ok));

      if (ok || no_input)
      {
         /* Run the C code for doing the actual search */
         switch (m_SearchTypes[m_CurrentTab])
         {
         case CLE_SEARCHTYPE_NORMAL:
            cl_search_step(
               &m_Searches[m_CurrentTab],
               no_input ? NULL : compare_value, 
               compare_size, 
               compare_type,
               is_float);
            break;
         case CLE_SEARCHTYPE_POINTER:
            cl_pointersearch_step(
               &m_PointerSearch[m_CurrentTab],
               no_input ? NULL : (uint32_t*)compare_value,
               compare_size,
               compare_type);
            break;
         default:
            cl_log("Search %u is of invalid type %u", m_CurrentTab, m_SearchTypes[m_CurrentTab]);
         }
      }
      else if (m_TextEntry->text().front() == '"' && m_TextEntry->text().back() == '"')
         cl_search_ascii(&m_Searches[m_CurrentTab], m_TextEntry->text().mid(1, m_TextEntry->text().length() - 2).toStdString().c_str(), m_TextEntry->text().length() - 2);
      else
         m_TextEntry->setText("");

      free(compare_value);
      rebuildRows();
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
         "The currently played game was not recognized by the server, so you cannot submit memory notes."
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

void CleMemoryInspector::onClickResultPointerSearch(void)
{
   if (m_ClickedResult < 0)
      return;
   else
   {
      cl_pointersearch_init(
         &m_PointerSearch[m_TabCount],
         getClickedResultAddress(),
         cl_sizeof_memtype(getCurrentSizeType()),
         1,
         0x100000,
         10000);

      m_Tabs->setCurrentIndex(m_TabCount);
      m_Tabs->setTabText(m_Tabs->currentIndex(), tr("Pointers"));
      m_Tabs->setTabTextColor(m_Tabs->currentIndex(), Qt::yellow);
      m_SearchTypes[m_Tabs->currentIndex()] = CLE_SEARCHTYPE_POINTER;

      m_ClickedResult = -1;
   }
}

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

void resultValueToString(char *string, uint8_t length, uint32_t value, 
   uint8_t memtype)
{
   switch (memtype)
   {
   case CL_MEMTYPE_8BIT:
      snprintf(string, length, "%02X (%u)", value, value);
      break;
   case CL_MEMTYPE_16BIT:
      snprintf(string, length, "%04X (%u)", value, value);
      break;
   case CL_MEMTYPE_32BIT:
      snprintf(string, length, "%08X (%u)", value, value);
      break;
   case CL_MEMTYPE_FLOAT:
      snprintf(string, length, "%08X (%f)", value, *((float*)(&value)));
      break;
   default:
      snprintf(string, length, "%08X", value);
      break;
   }
}

void CleMemoryInspector::rebuildRows()
{
   switch (m_SearchTypes[m_CurrentTab])
   {
   case CLE_SEARCHTYPE_NORMAL:
      rebuildRowsNormal();
      break;
   case CLE_SEARCHTYPE_POINTER:
      rebuildRowsPointer();
      break;
   }
}

void CleMemoryInspector::rebuildRowsNormal()
{
   cl_search_t *search;
   char         temp_string[32];
   uint8_t      memtype, size;
   uint32_t     current_row, matches, temp_value, i, j;

   search = &m_Searches[m_CurrentTab];

   /* (De)allocate rows */
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
         /* This value was filtered out */
         if (!m_Searches[m_CurrentTab].searchbanks[i].valid[j])
            continue;

         /* This value is still valid; add a new row */
         m_ResultTable->insertRow(current_row);
         /* Address */
         snprintf(temp_string, 256, "%08X", j + memory.banks[i].start);
         m_ResultTable->setItem(current_row, 0, new QTableWidgetItem(QString(temp_string)));
         /* Previous value */
         cl_read_search(&temp_value, &m_Searches[m_CurrentTab], &m_Searches[m_CurrentTab].searchbanks[i], j, size);
         resultValueToString(temp_string, sizeof(temp_string), temp_value, memtype);
         m_ResultTable->setItem(current_row, 1, new QTableWidgetItem(QString(temp_string)));
         /* Current value */
         cl_read_memory(&temp_value, &memory.banks[i], j, size);
         resultValueToString(temp_string, sizeof(temp_string), temp_value, memtype);
         m_ResultTable->setItem(current_row, 2, new QTableWidgetItem(QString(temp_string)));
         current_row++;

         /* No need to continue */
         if (current_row == matches)
         {
            m_ResultTable->setRowCount(matches);
            return;
         }
      }
   }
}

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

      resultValueToString(temp_string, sizeof(temp_string), search->results[i].value_previous, memtype);
      m_ResultTable->setItem(i, 2, new QTableWidgetItem(QString(temp_string)));

      resultValueToString(temp_string, sizeof(temp_string), search->results[i].value_current, memtype);
      m_ResultTable->setItem(i, 3, new QTableWidgetItem(QString(temp_string)));
   }
   m_ResultTable->setRowCount(matches);
}

void CleMemoryInspector::update()
{
   /* Don't change table data if we're editing it */
   if (m_CurrentEditedRow != -1)
      return;
   else
   {
      switch (m_SearchTypes[m_CurrentTab])
      {
      case CLE_SEARCHTYPE_NORMAL:
         updateNormal();
         break;
      case CLE_SEARCHTYPE_POINTER:
         updatePointer();
         break;
      }
   }
}

void CleMemoryInspector::updateNormal()
{
   QTableWidgetItem *item;
   char     temp_string[32];
   uint8_t  memtype, size;
   uint32_t address, curr_value, prev_value, i, j;

   memtype = getCurrentSizeType();
   size = cl_sizeof_memtype(getCurrentSizeType());

   for (i = 0; i < m_ResultTable->rowCount(); i++)
   {
      item = m_ResultTable->item(i, 0);
      if (!item)
         return;

      /* Don't visually update search results that are out of view */
      if (i < m_ResultTable->verticalScrollBar()->value())
         continue;
      else if (i > m_ResultTable->verticalScrollBar()->value() 
          + m_ResultTable->size().height() / 16)
         break;

      /* Kind of gross, but should save some memory 
         Only a few results should be redrawn at any time anyway */
      address = m_ResultTable->item(i, 0)->text().split(" ")[0].toULong(NULL, 16);

      if (!cl_read_memory(&curr_value, NULL, address, size) ||
          !cl_read_search(&prev_value, &m_Searches[m_CurrentTab], NULL, address, size))
         break;

      /* Update columns (besides the address) */
      /* Previous value column */
      item = m_ResultTable->item(i, 1);
      resultValueToString(temp_string, sizeof(temp_string), prev_value, memtype);
      item->setText(temp_string);

      /* Current value column */
      item = m_ResultTable->item(i, 2);
      if (m_CurrentEditedRow != i)
      {
         resultValueToString(temp_string, sizeof(temp_string), curr_value, memtype);
         item->setText(temp_string);
      }

      /* Display changed values in red */
      if (prev_value != curr_value)
         item->setTextColor(Qt::red);
      else
         item->setTextColor(Qt::white);
   }

   if (m_AddressOffset > memory.banks[0].start + memory.banks[0].size)
      return;

   memcpy(m_BufferCurrent, &memory.banks[0].data[m_AddressOffset], 256);
   m_HexWidget->refresh(m_BufferCurrent, m_BufferPrevious);
   memcpy(m_BufferPrevious, m_BufferCurrent, 256);
}

void CleMemoryInspector::updatePointer()
{
   QTableWidgetItem *item;
   cl_pointersearch_t *search = &m_PointerSearch[m_CurrentTab];
   char     temp_string[32];
   uint8_t  memtype, size;
   uint32_t address, curr_value, i, j;

   if (!search)
      return;

   memtype = getCurrentSizeType();
   size = cl_sizeof_memtype(getCurrentSizeType());

   for (i = 0; i < search->result_count; i++)
   {
      item = m_ResultTable->item(i, 0);
      if (!item)
         return;

      /* Don't visually update search results that are out of view */
      if (i < m_ResultTable->verticalScrollBar()->value())
         continue;
      else if (i > m_ResultTable->verticalScrollBar()->value() 
          + m_ResultTable->size().height() / 16)
         break;

      /* Kind of gross, but should save some memory 
         Only a few results should be redrawn at any time anyway */
      address = m_ResultTable->item(i, 0)->text().split(" ")[0].toULong(NULL, 16);

      for (j = 1; j <= search->passes; j++)
      {
         if (!cl_read_memory(&address, NULL, address, 4))
            break;
         address += m_ResultTable->item(i, j)->text().toULong(NULL, 16);
      }

      if (!cl_read_memory(&curr_value, NULL, address, size))
         continue;

      /* Update columns (besides the address) */
      /* Previous value column */
      item = m_ResultTable->item(i, j);
      resultValueToString(temp_string, sizeof(temp_string), search->results[i].value_previous, memtype);
      item->setText(temp_string);

      /* Current value column */
      item = m_ResultTable->item(i, j + 1);
      if (m_CurrentEditedRow != i)
      {
         resultValueToString(temp_string, sizeof(temp_string), curr_value, memtype);
         item->setText(temp_string);
      }

      /* Display changed values in red */
      if (search->results[i].value_previous != curr_value)
         item->setTextColor(Qt::red);
      else
         item->setTextColor(Qt::white);
   }
   m_ResultTable->setRowCount(search->result_count);

   if (m_AddressOffset > memory.banks[0].start + memory.banks[0].size)
      return;

   memcpy(m_BufferCurrent, &memory.banks[0].data[m_AddressOffset], 256);
   m_HexWidget->refresh(m_BufferCurrent, m_BufferPrevious);
   memcpy(m_BufferPrevious, m_BufferCurrent, 256);
}

#endif
