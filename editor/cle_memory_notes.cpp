#include "cle_memory_notes.h"

extern "C"
{
  #include "../cl_memory.h"
}

#include <QGridLayout>
#include <QHeaderView>
#include <QTableWidget>
#include <QString>

CleMemoryNotes::CleMemoryNotes(QWidget* parent) : QWidget(parent)
{
  m_Table = new QTableWidget(this);
  m_Table->setColumnCount(6);
  m_Table->setHorizontalHeaderLabels({
    "Key", "Address", "Type", "Value", "Offsets", "Title"
  });

  m_Table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
  m_Table->verticalHeader()->setVisible(false);
  m_Table->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_Table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_Table->setAlternatingRowColors(true);

  QGridLayout* layout = new QGridLayout(this);
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

    // Convert address and values to strings
    QString addr = QString("%1").arg((qulonglong)note->address, 8, 16, QChar('0')).toUpper();
    QString val  = QString::number(note->current.intval.raw);
    QString type = QString::number(note->type);
    QString key  = QString::number(note->key);
    QString offsets;
    QString title;

#if CL_HAVE_EDITOR
    title = note->details.title[0] ? note->details.title : "(No title)";
#else
    title = "(No title)";
#endif

    // Build offset string
    for (unsigned j = 0; j < note->pointer_passes; j++)
    {
      if (j > 0)
        offsets += " ";
      offsets += QString("0x%X").arg(note->pointer_offsets[j]);
    }

    // Populate row
    m_Table->setItem(row, 0, new QTableWidgetItem(key));
    m_Table->setItem(row, 1, new QTableWidgetItem(addr));
    m_Table->setItem(row, 2, new QTableWidgetItem(type));
    m_Table->setItem(row, 3, new QTableWidgetItem(val));
    m_Table->setItem(row, 4, new QTableWidgetItem(offsets));
    m_Table->setItem(row, 5, new QTableWidgetItem(title));
  }
}

void CleMemoryNotes::update(void)
{
  int rowCount = m_Table->rowCount();

  if ((unsigned)rowCount != memory.note_count)
    rebuild();
  for (unsigned i = 0; i < memory.note_count; i++)
  {
    const cl_memnote_t* note = &memory.notes[i];
    QTableWidgetItem* valueItem = m_Table->item(i, 3);
    if (valueItem)
    {
      if (note->type == CL_MEMTYPE_FLOAT || note->type == CL_MEMTYPE_DOUBLE)
        valueItem->setText(QString::number(note->current.floatval.fp));
      else
        valueItem->setText(QString::number(note->current.intval.raw));
    }
  }
}
