#include <QMessageBox>

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
      this, SLOT(onClickNew()));

   m_SearchButton = new QPushButton(tr("&Search"));
   m_SearchButton->setAutoDefault(true);
   connect(m_SearchButton, SIGNAL(clicked()), 
      this, SLOT(onClickSearch()));

   /* Initialize dropdown boxes */
   m_CompareDropdown = new QComboBox();
   m_CompareDropdown->addItem(tr("are equal to..."), CL_COMPARE_EQUAL);
   m_CompareDropdown->addItem(tr("are greater than..."), CL_COMPARE_GREATER);
   m_CompareDropdown->addItem(tr("are less than..."), CL_COMPARE_LESS);
   m_CompareDropdown->addItem(tr("are not equal to..."), CL_COMPARE_NOT_EQUAL);
   m_CompareDropdown->addItem(tr("have increased by..."), CL_COMPARE_INCREASED);
   m_CompareDropdown->addItem(tr("have decreased by..."), CL_COMPARE_DECREASED);
   connect(m_CompareDropdown, SIGNAL(activated(int)), 
      this, SLOT(onChangeCompareType()));

   m_SizeDropdown = new QComboBox();
   m_SizeDropdown->addItem(tr("1-byte values"), CL_MEMTYPE_UINT8);
   m_SizeDropdown->addItem(tr("2-byte values"), CL_MEMTYPE_UINT16);
   m_SizeDropdown->addItem(tr("4-byte values"), CL_MEMTYPE_UINT32);
   m_SizeDropdown->addItem(tr("float values"),  CL_MEMTYPE_FLOAT);
   m_SizeDropdown->addItem(tr("8-byte values"), CL_MEMTYPE_INT64);
   m_SizeDropdown->addItem(tr("double values"),  CL_MEMTYPE_DOUBLE);
   connect(m_SizeDropdown, SIGNAL(activated(int)),
      this, SLOT(onChangeSizeType()));

   /* Initialize text entry box for comparison value */
   m_TextEntry = new QLineEdit();
   connect(m_TextEntry, SIGNAL(textChanged(const QString&)),
      this, SLOT(onTargetChanged(const QString&)));
   connect(m_TextEntry, SIGNAL(returnPressed()), 
      m_SearchButton, SIGNAL(clicked()));

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

   /* Initialize scrollbar */
   m_Slider = new QSlider(Qt::Vertical, this);
   m_Slider->setInvertedAppearance(true);
   m_Slider->setTickInterval(1024 * 1024); //1MB
   m_Slider->setTickPosition(QSlider::TicksRight);
   connect(m_Slider, SIGNAL(valueChanged(int)),
           this, SLOT(onChangeScrollbar(int)));

   /* Initialize hex value view widget */
   m_HexWidget = new CleHexWidget(this, 1);
   m_BufferPrevious = (uint8_t*)malloc(256);
   m_BufferCurrent  = (uint8_t*)malloc(256);
   connect(m_HexWidget, SIGNAL(offsetEdited(cl_addr_t)), 
      this, SLOT(onAddressChanged(cl_addr_t)));
   connect(m_HexWidget, SIGNAL(valueEdited(cl_addr_t, uint64_t, uint8_t)),
      this, SLOT(onHexWidgetValueEdited(cl_addr_t, uint64_t, uint8_t)));
   connect(m_HexWidget, SIGNAL(requestAddMemoryNote(cl_addr_t)),
      this, SLOT(requestAddMemoryNote(cl_addr_t)));
   connect(m_HexWidget, SIGNAL(requestPointerSearch(cl_addr_t)),
      this, SLOT(requestPointerSearch(cl_addr_t)));
   if (memory.region_count > 0)
   {
     /* Block signals: m_RegionTabs doesn't exist yet, so offsetEdited must
        not fire until after the constructor finishes. */
     m_HexWidget->blockSignals(true);
     m_HexWidget->setByteSwapEnabled(memory.regions[0].endianness == CL_ENDIAN_BIG);
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
           this, &CleMemoryInspector::onChangeRegionTab);

   rebuildLayout();
   setWindowTitle(tr("Live Editor"));

   /* Initialize other variables */
   m_AddressOffset = 0;
   m_CurrentMembank = (memory.region_count > 0) ? &memory.regions[0] : nullptr;
   m_TabCount = 1;
   m_MemoryNoteSubmit = nullptr;

   onChangeCompareType();
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

void CleMemoryInspector::onAddressChanged(cl_addr_t address)
{
  cl_memory_region_t *new_bank = cl_find_memory_region(address);
  if (!new_bank)
    return;

  if (m_CurrentMembank != new_bank)
  {
    m_CurrentMembank = new_bank;
    m_CurrentRegionSize = new_bank->size;
    m_HexWidget->setRange(new_bank->base_guest,
                          new_bank->base_guest + new_bank->size + 1);
    m_Slider->setMinimum(0);
    m_Slider->setMaximum(new_bank->size - 256);

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

  m_AddressOffset = address - m_CurrentMembank->base_guest;
  m_HexWidget->setOffset(address);

  if (m_Slider->value() != m_AddressOffset)
    m_Slider->setValue(m_AddressOffset);
}

void CleMemoryInspector::onChangeCompareType(void)
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

void CleMemoryInspector::onChangeScrollbar(int value)
{
   onAddressChanged(m_CurrentMembank->base_guest + value);
}

void CleMemoryInspector::onChangeSizeType()
{
  m_CurrentSearch->setValueType(getCurrentSizeType());
  m_HexWidget->setSize(cl_sizeof_memtype(getCurrentSizeType()));
}

void CleMemoryInspector::onChangeTab(void)
{
  unsigned new_tab = m_Tabs->currentIndex();

  /* Was the "new tab" icon clicked? */
  if (new_tab >= m_TabCount)
  {
    /* Setup this tab to be a new search, add a "+" button */
    m_Tabs->setTabText(new_tab, tr("New Search"));
    m_Tabs->addTab("+");
    m_TabCount++;
    m_Searches[new_tab] = new CleResultTableNormal(this);
    m_Searches[new_tab]->setCompareType(getCurrentCompareType());
    m_Searches[new_tab]->setTarget(m_TextEntry->text());
    m_Searches[new_tab]->setValueType(getCurrentSizeType());
    m_SizeDropdown->setDisabled(false);
    m_TableStack->addWidget(m_Searches[new_tab]->table());
    applyTheme(m_Searches[new_tab]->table());
  }
  m_CurrentSearch = m_Searches[new_tab];
  m_CurrentSearch->rebuild();
  m_TableStack->setCurrentIndex(new_tab);

  m_CompareDropdown->setCurrentIndex(
    m_CompareDropdown->findData(m_CurrentSearch->compareType()));
  m_SizeDropdown->setCurrentIndex(
    m_SizeDropdown->findData(m_CurrentSearch->valueType()));
  m_HexWidget->setSize(cl_sizeof_memtype(getCurrentSizeType()));
}

void CleMemoryInspector::onClickNew()
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

void CleMemoryInspector::onClickSearch()
{
  if (!memory.region_count)
    return;
  else
  {
    if (!m_CurrentSearch->isInitted())
      onClickNew();
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

void CleMemoryInspector::onHexWidgetValueEdited(cl_addr_t address, uint64_t value, uint8_t size)
{
  cl_write_memory_value(&value, NULL, address, cl_pointer_type(size));
}

void CleMemoryInspector::onClickTabRename(void)
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

void CleMemoryInspector::onClickTabDelete()
{
  if (m_ClickedTab < 0 || m_ClickedTab >= m_TabCount || m_TabCount <= 1)
    return;

  m_TableStack->removeWidget(m_Searches[m_ClickedTab]->table());
  delete m_Searches[m_ClickedTab];

  for (int i = m_ClickedTab; i < m_TabCount - 1; i++)
    m_Searches[i] = m_Searches[i + 1];
  m_Searches[--m_TabCount] = nullptr;

  m_Tabs->removeTab(m_ClickedTab);
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
      QAction *action_delete = menu.addAction(tr("Delete"));
      action_delete->setEnabled(m_TabCount > 1);

      connect(action_rename, SIGNAL(triggered()), this,
        SLOT(onClickTabRename()));
      connect(action_delete, SIGNAL(triggered()), this,
        SLOT(onClickTabDelete()));

      menu.exec(m_Tabs->mapToGlobal(pos));
    }
  }
}

void CleMemoryInspector::onTargetChanged(const QString& target)
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

void CleMemoryInspector::requestAddMemoryNote(cl_addr_t address)
{
  cl_memnote_t note;

  note.address_initial = address;
  note.type = getCurrentSizeType();
  note.pointer_passes = 0;

  requestAddMemoryNote(note);
}

void CleMemoryInspector::requestPointerSearch(cl_addr_t address)
{
  if (!address)
    return;

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
    dlg.maxMatchesPerPass()
  );
  m_Searches[m_TabCount]->setCompareType(getCurrentCompareType());
  m_Searches[m_TabCount]->setTarget(m_TextEntry->text());

  m_TabCount++;
  m_TableStack->addWidget(m_Searches[m_TabCount-1]->table());
  applyTheme(m_Searches[m_TabCount-1]->table());
  m_Tabs->setCurrentIndex(m_TabCount - 1);
  m_Tabs->setTabText(m_Tabs->currentIndex(), tr("Pointers"));
  m_Tabs->setTabTextColor(m_Tabs->currentIndex(), QColor(200, 180, 255));
  m_Tabs->addTab("+");
  m_CurrentSearch = m_Searches[m_TabCount - 1];
  m_Status->setText(m_CurrentSearch->statusString());
}

void CleMemoryInspector::onChangeRegionTab(int index)
{
  if (index < 0 || (unsigned)index >= memory.region_count)
    return;

  cl_memory_region_t *region = &memory.regions[index];
  m_CurrentMembank = region;
  m_CurrentRegionSize = region->size;
  m_HexWidget->setByteSwapEnabled(region->endianness == CL_ENDIAN_BIG);
  m_HexWidget->setRange(region->base_guest, region->base_guest + region->size + 1);
  m_Slider->setMinimum(0);
  m_Slider->setMaximum(region->size - 256);
  m_AddressOffset = 0;
}

void CleMemoryInspector::run()
{
  /* Rebuild region tabs when region count changes */
  if (memory.region_count != m_CurrentRegionCount)
  {
    m_CurrentRegionCount = memory.region_count;
    m_RegionTabs->blockSignals(true);
    while (m_RegionTabs->count())
      m_RegionTabs->removeTab(0);
    for (unsigned i = 0; i < memory.region_count; i++)
    {
      const char *title = memory.regions[i].title;
      QString label = (title && title[0])
        ? QString(title)
        : QString("0x%1").arg(memory.regions[i].base_guest, 0, 16);
      m_RegionTabs->addTab(label);
    }
    m_RegionTabs->blockSignals(false);
    m_RegionTabs->setVisible(memory.region_count > 1);

    /* Memory regions just became available — initialize the active bank */
    if (!m_CurrentMembank && memory.region_count > 0)
    {
      m_CurrentMembank = &memory.regions[0];
      m_HexWidget->setByteSwapEnabled(m_CurrentMembank->endianness == CL_ENDIAN_BIG);
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
    m_Slider->setMinimum(0);
    m_Slider->setMaximum(m_CurrentMembank->size - 256);
  }

  m_CurrentSearch->run();
  cl_read_memory_buffer(m_BufferCurrent, nullptr, m_AddressOffset + m_CurrentMembank->base_guest, 256);
  m_HexWidget->refresh(m_BufferCurrent, m_BufferPrevious);
  memcpy(m_BufferPrevious, m_BufferCurrent, 256);
}
