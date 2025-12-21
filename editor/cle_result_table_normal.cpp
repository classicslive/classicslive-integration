#include <QMenu>
#include <QScrollBar>
#include <QStringList>

#include "cle_result_table_normal.h"
#include "cle_common.h"

extern "C" 
{
  #include "../cl_common.h"
}

#define COL_ADDRESS        0
#define COL_PREVIOUS_VALUE 1
#define COL_CURRENT_VALUE  2

CleResultTableNormal::CleResultTableNormal(QWidget *parent)
{
  CleResultTable::init();

  /* Normal-specific table styling */
  m_Table->setColumnCount(3);

  /* Initialize result table column headers */
  QStringList TableHeader;
  TableHeader << tr("Address") << tr("Previous") << tr("Current");
  m_Table->setHorizontalHeaderLabels(TableHeader);

  /* Qt connections to parent */
  connect(this, SIGNAL(addressChanged(cl_addr_t)),
    parent, SLOT(onAddressChanged(cl_addr_t)));
  connect(this, SIGNAL(requestAddMemoryNote(cl_memnote_t)),
    parent, SLOT(requestAddMemoryNote(cl_memnote_t)));
  connect(this, SIGNAL(requestPointerSearch(cl_addr_t)),
    parent, SLOT(requestPointerSearch(cl_addr_t)));

  cl_search_init(&m_Search);
}

CleResultTableNormal::~CleResultTableNormal(void)
{
  cl_search_free(&m_Search);
}

cl_addr_t CleResultTableNormal::getClickedResultAddress(void)
{
  return m_Table->item(m_Table->currentRow(), COL_ADDRESS)->text().split(" ")[0].toULong(NULL, 16);
}

void *CleResultTableNormal::searchData(void)
{
  return &m_Search;
}

void CleResultTableNormal::onResultClick(QTableWidgetItem *item) //todo
{
  CL_UNUSED(item);
  emit addressChanged(getClickedResultAddress() & ~0xF);
}

void CleResultTableNormal::onResultDoubleClick(void)
{
  if (m_Table->currentColumn() == COL_CURRENT_VALUE)
  {
    int i;

    /* We gray out the other entries because they won't update while
       we're editing. */
    for (i = 0; i < m_Table->rowCount(); i++)
      m_Table->item(i, COL_CURRENT_VALUE)->setForeground(Qt::gray);
    m_CurrentEditedRow = m_Table->currentRow();
  }
}

void CleResultTableNormal::onResultEdited(QTableWidgetItem *result)
{
  if (result->row() == m_CurrentEditedRow && result->column() == COL_CURRENT_VALUE)
  {
    if (result->isSelected())
      writeMemory(getClickedResultAddress(), m_Search.params, result->text());
    m_CurrentEditedRow = -1;
  }
}

void CleResultTableNormal::onClickResultAddMemoryNote(void)
{
  cl_memnote_t note;

  memset(&note, 0, sizeof(note));
  note.address_initial = getClickedResultAddress();
  note.type = m_Search.params.value_type;
  note.pointer_passes = 0;

  emit requestAddMemoryNote(note);
}

void CleResultTableNormal::onClickResultPointerSearch(void)
{
  emit requestPointerSearch(getClickedResultAddress());
}

void CleResultTableNormal::onClickResultRemove(void)
{
  if (cl_search_remove(&m_Search, getClickedResultAddress()) == CL_OK)
    rebuild();
}

void CleResultTableNormal::onResultRightClick(const QPoint& pos)
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
      QAction *action_ptr = menu.addAction(tr("Search for &pointers..."));
      QAction *action_remove = menu.addAction(tr("&Remove"));

      connect(action_add, SIGNAL(triggered()), this,
        SLOT(onClickResultAddMemoryNote()));
      connect(action_ptr, SIGNAL(triggered()), this,
        SLOT(onClickResultPointerSearch()));
      connect(action_remove, SIGNAL(triggered()), this,
        SLOT(onClickResultRemove()));

      menu.exec(m_Table->mapToGlobal(pos));
    }
  }
}

void CleResultTableNormal::onResultSelectionChanged(void)
{
  m_CurrentEditedRow = -1;
}

cl_error CleResultTableNormal::rebuild(void)
{
  char temp_string[32];
  int64_t temp_value = 0;
  unsigned val_size = m_Search.params.value_size;
  cl_value_type val_type = m_Search.params.value_type;
  unsigned current_row = 0;
  unsigned matches = m_Search.total_matches;
  unsigned char *chunk_buffer;

  if (matches == 0)
  {
    m_Table->setRowCount(0);
    return CL_OK;
  }
  if (matches > CLE_SEARCH_MAX_ROWS)
    matches = CLE_SEARCH_MAX_ROWS;

  m_Table->setRowCount(matches);
  chunk_buffer = reinterpret_cast<unsigned char*>(malloc(CL_SEARCH_CHUNK_SIZE));

  for (unsigned i = 0; i < m_Search.page_region_count; i++)
  {
    cl_search_page_t* page;

    /* Skip entire region if no matches */
    if (m_Search.page_regions[i].matches == 0)
      continue;
    page = m_Search.page_regions[i].first_page;
    while (page)
    {
      unsigned char *data = (unsigned char*)page->chunk;
      unsigned char *valid = (unsigned char*)page->validity;

      /* Skip entire page if no matches */
      if (page->matches == 0)
      {
        page = page->next;
        continue;
      }

      /* This page has matches, so copy a chunk from live memory for current values */
      cl_read_memory_buffer(chunk_buffer, page->region, 0, page->size);

      for (cl_addr_t offset = 0; offset < page->size; offset += val_size)
      {
        /* Skip value if not a match */
        if (!valid[offset / val_size])
          continue;

        /* Create a new row */
        m_Table->insertRow(current_row);

        /* Address */
        snprintf(temp_string, sizeof(temp_string), "%08X", (unsigned)(page->start + offset));
        m_Table->setItem(current_row, 0, new QTableWidgetItem(QString(temp_string)));

        /* Previous value (from chunk) */
        memcpy(&temp_value, data + offset, val_size);
        valueToString(temp_string, sizeof(temp_string), temp_value, val_type);
        m_Table->setItem(current_row, 1, new QTableWidgetItem(QString(temp_string)));

        /* Current value (from live memory) */
        cl_read_value(&temp_value, chunk_buffer, offset, val_type, page->region->endianness);
        valueToString(temp_string, sizeof(temp_string), temp_value, val_type);
        m_Table->setItem(current_row, 2, new QTableWidgetItem(QString(temp_string)));

        current_row++;
        if (current_row == matches)
        {
          m_Table->setRowCount(matches);
          free(chunk_buffer);
          return CL_OK;
        }
      }
      page = page->next;
    }
  }
  m_Table->setRowCount(matches);
  free(chunk_buffer);

  return CL_ERR_CLIENT_RUNTIME;
}

cl_error CleResultTableNormal::reset(void)
{
  cl_search_reset(&m_Search);
  m_Table->setRowCount(0);

  return CL_OK;
}

cl_error CleResultTableNormal::run(void)
{
  QTableWidgetItem *item;
  char temp_string[32];
  cl_value_type val_type = m_Search.params.value_type;
  cl_addr_t address = 0;
  int64_t curr_value = 0, prev_value = 0;
  int i;

  for (i = 0; i < m_Table->rowCount(); i++)
  {
    item = m_Table->item(i, 0);
    if (!item)
      continue;

    /* Only update rows that are visible */
    if (i < m_Table->verticalScrollBar()->value())
      continue;
    if (i > m_Table->verticalScrollBar()->value() + m_Table->height() / 16)
      break;

    /* Parse the address from the first column */
    address = item->text().split(" ")[0].toULong(NULL, 16);

    /* Get the two values */
    if (cl_read_memory_value(&curr_value, nullptr, address, val_type) != CL_OK ||
        cl_search_backup_value(&prev_value, &m_Search, address) != CL_OK)
      continue;

    /* Update previous value column */
    item = m_Table->item(i, COL_PREVIOUS_VALUE);
    if (item)
    {
      valueToString(temp_string, sizeof(temp_string), prev_value, val_type);
      item->setText(temp_string);
    }

    /* Update current value column */
    if (m_CurrentEditedRow < 0)
    {
      item = m_Table->item(i, COL_CURRENT_VALUE);
      if (item)
      {
        valueToString(temp_string, sizeof(temp_string), curr_value, val_type);
        item->setText(temp_string);

        /* Highlight changed values in red */
        item->setForeground(prev_value != curr_value ? Qt::red : Qt::white);
      }
    }
  }

  return CL_OK;
}

cl_error CleResultTableNormal::step(void)
{
  cl_error err = cl_search_step(&m_Search);

  if (err)
    return err;
  else
  {
    rebuild();
    return CL_OK;
  }
}
