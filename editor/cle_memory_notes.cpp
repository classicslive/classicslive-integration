#include "cle_memory_notes.h"

extern "C"
{
  #include "../cl_memory.h"
}

#include <QGridLayout>
#include <QHeaderView>
#include <QTableWidget>
#include <QString>

static const char *memtypeName(cl_value_type type)
{
  switch (type)
  {
  case CL_MEMTYPE_INT8:   return "int8";
  case CL_MEMTYPE_UINT8:  return "uint8";
  case CL_MEMTYPE_INT16:  return "int16";
  case CL_MEMTYPE_UINT16: return "uint16";
  case CL_MEMTYPE_INT32:  return "int32";
  case CL_MEMTYPE_UINT32: return "uint32";
  case CL_MEMTYPE_INT64:  return "int64";
  case CL_MEMTYPE_FLOAT:  return "float";
  case CL_MEMTYPE_DOUBLE: return "double";
  default:                return "?";
  }
}

static QString formatNoteValue(const cl_memnote_t *note)
{
  if (note->type == CL_MEMTYPE_FLOAT)
    return QString::number((double)note->current.floatval.fp, 'g', 6);
  else if (note->type == CL_MEMTYPE_DOUBLE)
    return QString::number(note->current.floatval.fp, 'g', 10);
  else
    return QString("0x%1 (%2)")
      .arg((qulonglong)note->current.intval.raw, 0, 16)
      .arg((qlonglong)note->current.intval.raw);
}

static QString formatAddress(const cl_memnote_t *note)
{
  QString text = QString::number((qulonglong)note->address_initial, 16).toUpper();

  if (note->pointer_passes == 0)
    return text;

  for (unsigned j = 0; j < note->pointer_passes; j++)
    text += QString(" + %1").arg((qulonglong)note->pointer_offsets[j], 0, 16).toUpper();

  text += QString(" = %1").arg(QString::number((qulonglong)note->address, 16).toUpper());
  return text;
}

CleMemoryNotes::CleMemoryNotes(QWidget* parent) : QWidget(parent)
{
  m_Table = new QTableWidget(this);
  m_Table->setColumnCount(5);
  m_Table->setHorizontalHeaderLabels({
    "Key", "Title", "Address", "Type", "Value"
  });

  m_Table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  m_Table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
  m_Table->verticalHeader()->setVisible(false);
  m_Table->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_Table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_Table->setAlternatingRowColors(true);
  m_Table->setShowGrid(false);
  m_Table->verticalHeader()->setDefaultSectionSize(22);
    connect(m_Table, SIGNAL(itemClicked(QTableWidgetItem*)),
      this, SLOT(onItemDoubleClicked(QTableWidgetItem*)));

  QGridLayout* layout = new QGridLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(m_Table, 0, 0);
  setLayout(layout);

  rebuild();
}

CleMemoryNotes::~CleMemoryNotes()
{
  delete m_Table;
}

void CleMemoryNotes::rebuild(void)
{
  m_Table->setRowCount(0);

  for (unsigned i = 0; i < memory.note_count; i++)
  {
    const cl_memnote_t* note = &memory.notes[i];
    int row = m_Table->rowCount();
    m_Table->insertRow(row);

    QString addr  = formatAddress(note);
    QString title = note->details.title[0] ? note->details.title : "(No title)";

    m_Table->setItem(row, 0, new QTableWidgetItem(QString::number(note->key)));
    m_Table->setItem(row, 1, new QTableWidgetItem(title));
    m_Table->setItem(row, 2, new QTableWidgetItem(addr));
    m_Table->setItem(row, 3, new QTableWidgetItem(memtypeName(note->type)));
    m_Table->setItem(row, 4, new QTableWidgetItem(formatNoteValue(note)));
  }
}

void CleMemoryNotes::update(void)
{
  if ((unsigned)m_Table->rowCount() != memory.note_count)
  {
    rebuild();
    return;
  }

  for (unsigned i = 0; i < memory.note_count; i++)
  {
    const cl_memnote_t* note = &memory.notes[i];
    const cl_memnote_ex_value_t *value = &note->details.values[0];
    QTableWidgetItem* addressItem = m_Table->item(i, 2);
    QTableWidgetItem* valueItem = m_Table->item(i, 4);

    if (!addressItem || !valueItem)
      continue;

    addressItem->setText(formatAddress(note));

    QString text = formatNoteValue(note);

    /* Append known value label from description if matched */
    while (value->title[0] != '\0')
    {
      if (note->current.intval.raw == value->value)
      {
        text += QString(" (%1)").arg(value->title);
        break;
      }
      value++;
    }

    valueItem->setText(text);
  }
}

void CleMemoryNotes::onItemDoubleClicked(QTableWidgetItem *item)
{
  if (!item)
    return;

  const int address_column = 2;
  int row = item->row();
  int col = item->column();

  if (col != address_column)
    return;
  else if (row < 0 || (unsigned)row >= memory.note_count)
    return;
  else
  {
    const cl_memnote_t *note = &memory.notes[row];
    cl_addr_t address = note->address ? note->address : note->address_initial;

    emit noteActivated(*note);
    emit addressActivated(address);
  }
}
