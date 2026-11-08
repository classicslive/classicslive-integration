#include <QMessageBox>

#include <climits>

#include "cle_active_regions_dialog.h"
#include "cle_memory_inspector.h"
#include "cle_pointer_search_dialog.h"
#include "cle_result_table_normal.h"
#include "cle_result_table_pointer.h"

extern "C"
{
   #include "../cl_common.h"
   #include "../cl_main.h"
}

void CleMemoryInspector::applyTheme(QWidget *w)
{
  w->setStyle(style());
  w->setPalette(palette());
  for (QWidget *child : w->findChildren<QWidget *>())
  {
    child->setStyle(style());
    child->setPalette(palette());
  }
}

void CleMemoryInspector::rebuildLayout()
{
  if (layout())
    delete layout();

  m_Layout = new QGridLayout(this);

  /* Initialize window layout */
  m_Layout->setMenuBar(m_MenuBar);
  m_Layout->addWidget(m_Tabs,            0, 0, 1, 4);
  m_Layout->addWidget(m_SizeDropdown,    1, 0, 1, 1);
  m_Layout->addWidget(m_CompareDropdown, 1, 1, 1, 1);
  m_Layout->addWidget(m_TextEntry,       1, 2, 1, 1);
  m_Layout->addWidget(m_SearchButton,    1, 3, 1, 1);
  m_Layout->addWidget(m_TableStack,      2, 0, 2, 4);
  m_Layout->addWidget(m_HexWidget,       4, 0, 2, 3);
  m_Layout->addWidget(m_Slider,          4, 3, 2, 1);
  m_Layout->addWidget(m_RegionTabs,      6, 0, 1, 4);
  m_Layout->addWidget(m_Status,          7, 0, 1, 3);
  m_Layout->addWidget(m_NewButton,       7, 3, 1, 1);
  setLayout(m_Layout);
}

CleMemoryInspector::CleMemoryInspector()
{
   /* Initialize basic elements */
   m_NewButton = new QPushButton(tr("&New Search"));
   connect(m_NewButton, SIGNAL(clicked()),
      this, SLOT(clickNew()));

   m_SearchButton = new QPushButton(tr("&Search"));
   m_SearchButton->setAutoDefault(true);
   connect(m_SearchButton, SIGNAL(clicked()),
      this, SLOT(clickSearch()));

   /* Initialize dropdown boxes */
   m_CompareDropdown = new QComboBox();
   m_CompareDropdown->addItem(tr("are equal to..."), CL_COMPARE_EQUAL);
   m_CompareDropdown->addItem(tr("are greater than..."), CL_COMPARE_GREATER);
   m_CompareDropdown->addItem(tr("are less than..."), CL_COMPARE_LESS);
   m_CompareDropdown->addItem(tr("are not equal to..."), CL_COMPARE_NOT_EQUAL);
   m_CompareDropdown->addItem(tr("have increased by..."), CL_COMPARE_INCREASED);
   m_CompareDropdown->addItem(tr("have decreased by..."), CL_COMPARE_DECREASED);
   connect(m_CompareDropdown, SIGNAL(activated(int)),
      this, SLOT(changeCompareType()));

   m_SizeDropdown = new QComboBox();
   m_SizeDropdown->addItem(tr("1-byte values"), CL_MEMTYPE_UINT8);
   m_SizeDropdown->addItem(tr("2-byte values"), CL_MEMTYPE_UINT16);
   m_SizeDropdown->addItem(tr("4-byte values"), CL_MEMTYPE_UINT32);
   m_SizeDropdown->addItem(tr("float values"),  CL_MEMTYPE_FLOAT);
   m_SizeDropdown->addItem(tr("8-byte values"), CL_MEMTYPE_INT64);
   m_SizeDropdown->addItem(tr("double values"),  CL_MEMTYPE_DOUBLE);
   connect(m_SizeDropdown, SIGNAL(activated(int)),
      this, SLOT(changeSizeType()));

   /* Initialize text entry box for comparison value */
   m_TextEntry = new QLineEdit();
   connect(m_TextEntry, SIGNAL(textChanged(const QString&)),
      this, SLOT(targetChanged(const QString&)));
   connect(m_TextEntry, SIGNAL(returnPressed()),
      m_SearchButton, SIGNAL(clicked()));

   /* Initialize tab view for switching between searches */
   m_Tabs = new QTabBar();
   m_Tabs->setContextMenuPolicy(Qt::CustomContextMenu);
   m_Tabs->setExpanding(false);
   m_Tabs->addTab(tr("Search 1"));
   m_Tabs->addTab(tr("+"));
   connect(m_Tabs, SIGNAL(currentChanged(int)),
      this, SLOT(changeTab()));
   connect(m_Tabs, SIGNAL(customContextMenuRequested(const QPoint&)),
      this, SLOT(rightClickTabs(const QPoint&)));

   /* Initialize scrollbar */
   m_Slider = new QSlider(Qt::Vertical, this);
   m_Slider->setInvertedAppearance(true);
   m_Slider->setTickPosition(QSlider::TicksRight);
   connect(m_Slider, SIGNAL(valueChanged(int)),
           this, SLOT(changeScrollbar(int)));

   /* Initialize hex value view widget */
   m_HexWidget = new CleHexWidget(this, 1);
   m_BufferPrevious = (uint8_t*)malloc(256);
   m_BufferCurrent  = (uint8_t*)malloc(256);
   connect(m_HexWidget, SIGNAL(offsetEdited(cl_addr_t)),
      this, SLOT(addressChanged(cl_addr_t)));
   connect(m_HexWidget, SIGNAL(valueEdited(cl_addr_t, uint64_t, uint8_t)),
      this, SLOT(hexWidgetValueEdited(cl_addr_t, uint64_t, uint8_t)));
   connect(m_HexWidget, SIGNAL(requestAddMemoryNote(cl_memnote_t)),
      this, SLOT(requestAddMemoryNote(cl_memnote_t)));
   connect(m_HexWidget, SIGNAL(requestPointerSearch(cl_addr_t)),
      this, SLOT(requestPointerSearch(cl_addr_t)));
   if (memory.region_count > 0)
   {
     /* Block signals: m_RegionTabs doesn't exist yet, so offsetEdited must
        not fire until after the constructor finishes. */
     m_HexWidget->blockSignals(true);
     m_HexWidget->setByteSwapEnabled(memory.regions[0].endianness == CL_ENDIAN_BIG);
     m_HexWidget->setWordFlipEnabled(memory.regions[0].endianness == CL_ENDIAN_WORD_FLIP_BL);
     m_HexWidget->setOffset(memory.regions[0].base_guest);
     m_HexWidget->setRange(memory.regions[0].base_guest, memory.regions[0].base_guest + memory.regions[0].size + 1);
     m_HexWidget->blockSignals(false);
   }

  /* Initialize status footer */
  m_Status = new QLabel(tr("Matches: 0 | Scanned: 0 MB | Scan time: 0.00 s"));
  m_Status->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

   /* Initialize timer for updating search rows */
   m_UpdateTimer = new QTimer(this);
   connect(m_UpdateTimer, SIGNAL(timeout()), this, SLOT(run()));
   m_UpdateTimer->start(100);

   memset(m_Searches, 0, sizeof(m_Searches));
   memset(m_TabPositions, 0, sizeof(m_TabPositions));
   m_Searches[0] = new CleResultTableNormal(this);
   m_Searches[0]->setCompareType(getCurrentCompareType());
   m_Searches[0]->setTarget(m_TextEntry->text());
   m_Searches[0]->setValueType(getCurrentSizeType());
   m_CurrentSearch = m_Searches[0];

   m_TableStack = new QStackedWidget(this);
   m_TableStack->addWidget(m_CurrentSearch->table());

   m_RegionTabs = new QTabBar();
   m_RegionTabs->setExpanding(false);
   m_RegionTabs->hide();
   connect(m_RegionTabs, &QTabBar::currentChanged,
           this, &CleMemoryInspector::changeRegionTab);

   /* Build menu bar */
   m_MenuBar = new QMenuBar(this);
   QMenu *memoryMenu = m_MenuBar->addMenu(tr("Memory"));
   QAction *activeRegionsAction = memoryMenu->addAction(tr("Active regions..."));
   connect(activeRegionsAction, &QAction::triggered,
           this, &CleMemoryInspector::showActiveRegions);

   rebuildLayout();
   setWindowTitle(tr("Live Editor"));

   /* Initialize other variables */
   m_AddressOffset = 0;
   m_CurrentMembank = (memory.region_count > 0) ? &memory.regions[0] : nullptr;
   m_TabCount = 1;
   m_ActiveTab = 0;
   m_MemoryNoteSubmit = nullptr;

   changeCompareType();
}

cl_compare_type CleMemoryInspector::getCurrentCompareType(void)
{
  return static_cast<cl_compare_type>(m_CompareDropdown->itemData(
    m_CompareDropdown->currentIndex()).toUInt());
}

cl_value_type CleMemoryInspector::getCurrentSizeType(void)
{
   return static_cast<cl_value_type>(m_SizeDropdown->itemData(m_SizeDropdown->currentIndex()).toUInt());
}

void CleMemoryInspector::bringToFront()
{
  show();
  raise();
  activateWindow();
  setFocus(Qt::OtherFocusReason);
}

int CleMemoryInspector::regionIndex(const cl_memory_region_t *region) const
{
  unsigned i;

  for (i = 0; i < memory.region_count; i++)
    if (&memory.regions[i] == region)
      return static_cast<int>(i);

  return -1;
}

/**
 * Shows the "new search" tab only while there is room for another search.
 * It always sits last, just past the searches themselves.
 */
void CleMemoryInspector::updateNewTabButton(void)
{
  const bool room  = m_TabCount < CLE_MAX_TABS;
  const bool shown = m_Tabs->count() > (int)m_TabCount;

  if (room == shown)
    return;
  else if (room)
    m_Tabs->addTab("+");
  else
    m_Tabs->removeTab((int)m_TabCount);
}

/* Park the search tab we are leaving so returning to it restores the cursor */
void CleMemoryInspector::saveTabPosition(void)
{
  if (m_ActiveTab < 0 || m_ActiveTab >= CLE_MAX_TABS || !m_CurrentMembank)
    return;

  m_TabPositions[m_ActiveTab].view =
    m_CurrentMembank->base_guest + m_AddressOffset;
  m_TabPositions[m_ActiveTab].cursor = m_HexWidget->cursorOffset();
  m_TabPositions[m_ActiveTab].valid = true;
}

/* Park the region we are leaving so returning to it restores the view */
void CleMemoryInspector::saveRegionOffset(void)
{
  int index = regionIndex(m_CurrentMembank);

  if (index >= 0 && static_cast<size_t>(index) < m_RegionOffsets.size())
    m_RegionOffsets[index] = m_AddressOffset;
}

void CleMemoryInspector::addressChanged(cl_addr_t address)
{
  address &= ~((cl_addr_t)0xF);
  cl_memory_region_t *new_bank = cl_find_memory_region(address);
  if (!new_bank)
    return;

  if (m_CurrentMembank != new_bank)
  {
    saveRegionOffset();
    m_CurrentMembank = new_bank;
    m_CurrentRegionSize = new_bank->size;
    m_HexWidget->setRange(new_bank->base_guest,
                          new_bank->base_guest + new_bank->size + 1);
    setSliderRange(new_bank->size);

    /* Sync the region tab to the newly active bank */
    for (unsigned i = 0; i < memory.region_count; i++)
    {
      if (&memory.regions[i] == new_bank)
      {
        m_RegionTabs->blockSignals(true);
        m_RegionTabs->setCurrentIndex(static_cast<int>(i));
        m_RegionTabs->blockSignals(false);
        break;
      }
    }
  }

  m_AddressOffset = (address - m_CurrentMembank->base_guest) & ~(cl_addr_t)0xF;
  m_HexWidget->setOffset(address);

  setSliderOffset(m_AddressOffset);
}

void CleMemoryInspector::changeCompareType(void)
{
  switch (getCurrentCompareType())
  {
  case CL_COMPARE_EQUAL:
  case CL_COMPARE_GREATER:
  case CL_COMPARE_LESS:
  case CL_COMPARE_NOT_EQUAL:
    m_TextEntry->setPlaceholderText(tr("previous value"));
    break;
  case CL_COMPARE_INCREASED:
  case CL_COMPARE_DECREASED:
    m_TextEntry->setPlaceholderText(tr("any amount"));
    break;
  default:
    m_TextEntry->setPlaceholderText("");
  }
  m_CurrentSearch->setCompareType(getCurrentCompareType());
}

void CleMemoryInspector::changeScrollbar(int value)
{
   addressChanged(m_CurrentMembank->base_guest +
                  (static_cast<cl_addr_t>(value) << m_SliderShift));
}

void CleMemoryInspector::setSliderRange(cl_addr_t size)
{
  cl_addr_t span = size > 256 ? size - 256 : 0;

  m_SliderShift = 4;
  while ((span >> m_SliderShift) > INT_MAX)
    m_SliderShift++;

  m_Slider->blockSignals(true);
  m_Slider->setRange(0, static_cast<int>(span >> m_SliderShift));
  m_Slider->setTickInterval(qMax(1, static_cast<int>(
    (size > CL_GB(1) ? CL_GB(1) : CL_MB(1)) >> m_SliderShift)));
  m_Slider->blockSignals(false);
}

/* Moves the slider without it moving the view back */
void CleMemoryInspector::setSliderOffset(cl_addr_t offset)
{
  int value = static_cast<int>(offset >> m_SliderShift);

  if (m_Slider->value() != value)
  {
    m_Slider->blockSignals(true);
    m_Slider->setValue(value);
    m_Slider->blockSignals(false);
  }
}

void CleMemoryInspector::changeSizeType()
{
  cl_value_type type = getCurrentSizeType();
  m_CurrentSearch->setValueType(type);
  m_HexWidget->setValueType(type);
  m_HexWidget->setSize(cl_sizeof_memtype(type));
}

void CleMemoryInspector::changeTab(void)
{
  unsigned new_tab = m_Tabs->currentIndex();

  saveTabPosition();

  /* Was the "new tab" icon clicked? */
  if (new_tab >= m_TabCount)
  {
    if (m_TabCount >= CLE_MAX_TABS)
    {
      /* No room left, so stay on the tab we were already showing */
      m_Tabs->setCurrentIndex(m_ActiveTab >= 0 ? m_ActiveTab : 0);
      return;
    }

    /* Setup this tab to be a new search */
    m_Tabs->setTabText(new_tab, tr("New Search"));
    m_TabCount++;
    m_Searches[new_tab] = new CleResultTableNormal(this);
    m_Searches[new_tab]->setCompareType(getCurrentCompareType());
    m_Searches[new_tab]->setTarget(m_TextEntry->text());
    m_Searches[new_tab]->setValueType(getCurrentSizeType());
    m_SizeDropdown->setDisabled(false);
    m_TableStack->addWidget(m_Searches[new_tab]->table());
    applyTheme(m_Searches[new_tab]->table());
    updateNewTabButton();

    /* A new tab starts out wherever the view already is */
    m_TabPositions[new_tab].valid = false;
  }
  m_CurrentSearch = m_Searches[new_tab];
  m_CurrentSearch->rebuild();
  m_TableStack->setCurrentIndex(new_tab);

  m_CompareDropdown->setCurrentIndex(
    m_CompareDropdown->findData(m_CurrentSearch->compareType()));
  m_SizeDropdown->setCurrentIndex(
    m_SizeDropdown->findData(m_CurrentSearch->valueType()));
  m_HexWidget->setValueType(getCurrentSizeType());
  m_HexWidget->setSize(cl_sizeof_memtype(getCurrentSizeType()));

  /* Put the hex view back where this tab left it. The size change above
     re-aligns the cursor, so this has to follow it. */
  if (new_tab < CLE_MAX_TABS && m_TabPositions[new_tab].valid &&
      cl_find_memory_region(m_TabPositions[new_tab].view))
  {
    addressChanged(m_TabPositions[new_tab].view);
    m_HexWidget->setCursorOffset(m_TabPositions[new_tab].cursor);
  }
  m_ActiveTab = (int8_t)new_tab;
}

void CleMemoryInspector::clickNew()
{
  if (!memory.region_count)
    return;
  else
  {
    m_TableStack->removeWidget(m_Searches[m_Tabs->currentIndex()]->table());
    delete m_Searches[m_Tabs->currentIndex()];
    m_Searches[m_Tabs->currentIndex()] = new CleResultTableNormal(this);
    m_CurrentSearch = m_Searches[m_Tabs->currentIndex()];

    m_TableStack->insertWidget(m_Tabs->currentIndex(), m_CurrentSearch->table());
    m_TableStack->setCurrentWidget(m_CurrentSearch->table());
    applyTheme(m_CurrentSearch->table());
    m_Tabs->setTabTextColor(m_Tabs->currentIndex(), Qt::white);

    m_CurrentSearch->setCompareType(getCurrentCompareType());
    m_CurrentSearch->setValueType(getCurrentSizeType());
    m_CurrentSearch->setTarget(m_TextEntry->text());

    m_SizeDropdown->setDisabled(false);
  }
}

void CleMemoryInspector::clickSearch()
{
  if (!memory.region_count)
    return;
  else
  {
    if (!m_CurrentSearch->isInitted())
      clickNew();
    if (m_CurrentSearch->step() != CL_OK)
    {
      cl_log("Search step failed: %s\n", m_TextEntry->text().toStdString().c_str());
      m_TextEntry->setText("");
    }
    else
    {
      m_SizeDropdown->setDisabled(true);
      m_Status->setText(m_CurrentSearch->statusString());
    }
  }
}

void CleMemoryInspector::focusedNoteChanged(cl_memnote_t note)
{
  m_HexWidget->setFocusedNote(&note);
}

void CleMemoryInspector::hexWidgetValueEdited(cl_addr_t address, uint64_t value, uint8_t size)
{
  /* Write the value back exactly the way the view is reading it. The view's
     own flags are what to follow here, not the region's endianness, as the
     'S' and 'N' keys let the two disagree. */
  const bool flip = m_HexWidget->wordFlipEnabled();
  const bool big = m_HexWidget->viewEndianness() == CL_ENDIAN_BIG;
  cl_addr_t offset;
  uint8_t i;

  if (!m_CurrentMembank)
    return;

  offset = address - m_CurrentMembank->base_guest;

  for (i = 0; i < size; i++)
  {
    /* Most significant byte first when the view reads big-endian */
    uint8_t byte = (uint8_t)(value >> ((big ? size - 1 - i : i) * 8));
    cl_addr_t at = flip ? ((offset + i) ^ 3) : (offset + i);

    cl_write_memory_buffer(&byte, m_CurrentMembank, at, 1);
  }
}

void CleMemoryInspector::clickTabRename(void)
{
  QString text = QInputDialog::getText
  (
    this,
    tr("Rename"),
    tr("Search tab name:")
  );

  if (m_ClickedTab >= 0 && m_ClickedTab < m_TabCount && !text.isEmpty())
    m_Tabs->setTabText(m_ClickedTab, text);
}

void CleMemoryInspector::clickTabDelete()
{
  if (m_ClickedTab < 0 || m_ClickedTab >= m_TabCount || m_TabCount <= 1)
    return;

  m_TableStack->removeWidget(m_Searches[m_ClickedTab]->table());
  delete m_Searches[m_ClickedTab];

  for (int i = m_ClickedTab; i < m_TabCount - 1; i++)
  {
    m_Searches[i] = m_Searches[i + 1];
    m_TabPositions[i] = m_TabPositions[i + 1];
  }
  m_Searches[--m_TabCount] = nullptr;
  m_TabPositions[m_TabCount].valid = false;

  /* Keep the active tab pointing at the same search after the shift */
  if (m_ActiveTab == m_ClickedTab)
    m_ActiveTab = -1;
  else if (m_ActiveTab > m_ClickedTab)
    m_ActiveTab--;

  m_Tabs->removeTab(m_ClickedTab);
  updateNewTabButton();
}

void CleMemoryInspector::rightClickTabs(const QPoint &pos)
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
      QAction *action_delete = menu.addAction(tr("Delete"));
      action_delete->setEnabled(m_TabCount > 1);

      connect(action_rename, SIGNAL(triggered()), this,
        SLOT(clickTabRename()));
      connect(action_delete, SIGNAL(triggered()), this,
        SLOT(clickTabDelete()));

      menu.exec(m_Tabs->mapToGlobal(pos));
    }
  }
}

void CleMemoryInspector::targetChanged(const QString& target)
{
  if (m_CurrentSearch->setTarget(target) != CL_OK)
  {
    cl_log("Search input failed: %s\n", target.toStdString().c_str());
    m_TextEntry->setStyleSheet("color: gray;");
  }
  else
    m_TextEntry->setStyleSheet("");
}

void CleMemoryInspector::requestAddMemoryNote(cl_memnote_t note)
{
  if (!session.game_id)
  {
    QMessageBox::warning(this, "Live Editor",
      tr("The content you are running was not recognized by the server, so "
          "you cannot submit memory notes.")
     );
  }
  else
  {
    m_MemoryNoteSubmit = new CleMemoryNoteSubmit(note);
    m_MemoryNoteSubmit->show();
  }
}

void CleMemoryInspector::showActiveRegions()
{
  CleActiveRegionsDialog dlg(this);
  dlg.exec();
}

void CleMemoryInspector::requestPointerSearch(cl_addr_t address)
{
  if (!address)
    return;
  else if (m_TabCount >= CLE_MAX_TABS)
  {
    QMessageBox::warning(this, "Live Editor",
      tr("All search tabs are in use. Close one to start a pointer search."));
    return;
  }

  ClePointerSearchDialog dlg(this);
  if (dlg.exec() != QDialog::Accepted)
    return;

  m_Searches[m_TabCount] = new CleResultTablePointer(
    this,
    address,
    getCurrentSizeType(),
    dlg.pointerFollows(),
    dlg.offsetRange(),
    dlg.maxMatches(),
    dlg.maxMatchesPerPass(),
    dlg.baseRegion()
  );
  m_Searches[m_TabCount]->setCompareType(getCurrentCompareType());
  m_Searches[m_TabCount]->setTarget(m_TextEntry->text());

  m_TabCount++;
  m_TabPositions[m_TabCount - 1].valid = false;
  m_TableStack->addWidget(m_Searches[m_TabCount-1]->table());
  applyTheme(m_Searches[m_TabCount-1]->table());
  m_Tabs->setCurrentIndex(m_TabCount - 1);
  m_Tabs->setTabText(m_Tabs->currentIndex(), tr("Pointers"));
  m_Tabs->setTabTextColor(m_Tabs->currentIndex(), QColor(200, 180, 255));
  updateNewTabButton();
  m_CurrentSearch = m_Searches[m_TabCount - 1];
  m_Status->setText(m_CurrentSearch->statusString());
}

void CleMemoryInspector::changeRegionTab(int index)
{
  if (index < 0 || (unsigned)index >= memory.region_count)
    return;
  else
  {
    cl_memory_region_t *region = &memory.regions[index];
    cl_addr_t offset = 0;

    saveRegionOffset();

    m_CurrentMembank = region;
    m_CurrentRegionSize = region->size;
    m_HexWidget->setByteSwapEnabled(region->endianness == CL_ENDIAN_BIG);
    m_HexWidget->setWordFlipEnabled(region->endianness == CL_ENDIAN_WORD_FLIP_BL);
    m_HexWidget->setRange(region->base_guest, region->base_guest + region->size + 1);
    setSliderRange(region->size);

    /* Restore where we last were in this region, if it still fits */
    if (static_cast<size_t>(index) < m_RegionOffsets.size())
      offset = m_RegionOffsets[index];
    if (offset + 256 > region->size)
      offset = 0;

    m_AddressOffset = offset;
    m_HexWidget->setOffset(region->base_guest + offset);
    setSliderOffset(offset);
  }
}

void CleMemoryInspector::run()
{
  /* Rebuild region tabs when region count changes */
  if (memory.region_count != m_CurrentRegionCount)
  {
    m_CurrentRegionCount = memory.region_count;
    m_RegionOffsets.assign(memory.region_count, 0);
    m_RegionTabs->blockSignals(true);
    while (m_RegionTabs->count())
      m_RegionTabs->removeTab(0);
    int firstActive = -1;
    for (unsigned i = 0; i < memory.region_count; i++)
    {
      const cl_memory_region_t *region = &memory.regions[i];
      const char *title = region->title;
      QString label = (title && title[0])
        ? QString(title)
        : QString("0x%1").arg(region->base_guest, 0, 16);
      m_RegionTabs->addTab(label);

      if (region->ignore)
        m_RegionTabs->setTabTextColor(static_cast<int>(i),
          palette().color(QPalette::Disabled, QPalette::Text));

      if (firstActive < 0 && !region->ignore)
        firstActive = static_cast<int>(i);
    }
    m_RegionTabs->blockSignals(false);
    m_RegionTabs->setVisible(memory.region_count > 1);

    /* Memory regions just became available — seek to the first active region */
    if (!m_CurrentMembank && memory.region_count > 0)
    {
      int target = firstActive >= 0 ? firstActive : 0;
      m_RegionTabs->setCurrentIndex(target);
      m_CurrentMembank = &memory.regions[target];
      m_HexWidget->setByteSwapEnabled(m_CurrentMembank->endianness == CL_ENDIAN_BIG);
      m_HexWidget->setWordFlipEnabled(m_CurrentMembank->endianness == CL_ENDIAN_WORD_FLIP_BL);
      m_HexWidget->setOffset(m_CurrentMembank->base_guest);
    }
  }

  if (!m_CurrentMembank)
    return;

  /* Detect region size changes (e.g. when a game is loaded after init) */
  if (m_CurrentMembank->size != m_CurrentRegionSize)
  {
    m_CurrentRegionSize = m_CurrentMembank->size;
    m_HexWidget->setRange(m_CurrentMembank->base_guest,
                          m_CurrentMembank->base_guest + m_CurrentMembank->size + 1);
    setSliderRange(m_CurrentMembank->size);
    setSliderOffset(m_AddressOffset);
  }

  m_CurrentSearch->run();
  cl_read_memory_buffer(m_BufferCurrent, nullptr, m_AddressOffset + m_CurrentMembank->base_guest, 256);
  m_HexWidget->refresh(m_BufferCurrent, m_BufferPrevious);
  memcpy(m_BufferPrevious, m_BufferCurrent, 256);
}
