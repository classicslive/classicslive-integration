#include "cle_memory_note_submit.h"

CleMemoryNoteSubmit::CleMemoryNoteSubmit(cl_memnote_t note)
{
  QGridLayout *Layout = new QGridLayout(this);
  char footer_text[256];

  m_MemoryNote = note;

  m_CancelButton = new QPushButton(tr("Cancel"), this);
  connect(m_CancelButton, SIGNAL(clicked()), this, SLOT(onClickCancel()));
  m_SubmitButton = new QPushButton(tr("Submit"), this);
  connect(m_SubmitButton, SIGNAL(clicked()), this, SLOT(onClickSubmit()));

  m_Title = new QLineEdit(this);
  m_Title->setPlaceholderText(tr("Title"));

  m_Description = new QTextEdit(this);
  m_Description->setPlaceholderText(tr("Description (optional)"));

  m_Footer = new QLabel(this);
  snprintf(footer_text, sizeof(footer_text), "0x%016llX - %u pointer passes",
    note.address, note.pointer_passes);
  m_Footer->setText(footer_text);

  Layout->addWidget(m_Title,        0, 0, 1, 2);
  Layout->addWidget(m_Description,  1, 0, 1, 2);
  Layout->addWidget(m_Footer,       2, 0, 1, 2);
  Layout->addWidget(m_CancelButton, 3, 0, 1, 1);
  Layout->addWidget(m_SubmitButton, 3, 1, 1, 1);

  setLayout(Layout);
  setWindowFlags(Qt::WindowTitleHint);
  setWindowTitle(tr("Add memory note"));
}

CleMemoryNoteSubmit::~CleMemoryNoteSubmit(void)
{
  cl_free_memnote(&m_MemoryNote);
}

void CleMemoryNoteSubmit::onClickCancel(void)
{
  close();
}

void CleMemoryNoteSubmit::onClickSubmit(void)
{
  if (m_Title->text().isEmpty())
    return;
  else
  {
    QByteArray post;
    post.append("game_id=" + QByteArray::number(session.game_id));
    post.append("&address=" + QByteArray::number((qulonglong)m_MemoryNote.address));
    post.append("&type=" + QByteArray::number(m_MemoryNote.type));

    if (m_MemoryNote.pointer_passes)
    {
      QStringList offsets;
      for (unsigned i = 0; i < m_MemoryNote.pointer_passes; ++i)
        offsets << QString::number(m_MemoryNote.pointer_offsets[i]);
      post.append("&offsets=" + QUrl::toPercentEncoding(offsets.join(" ")));
    }

    QString title = m_Title->text();
    QString description = m_Description->toPlainText();

    post.append("&title=" + QUrl::toPercentEncoding(title));

    if (!description.isEmpty())
      post.append("&description=" + QUrl::toPercentEncoding(description));

    cl_network_post_api(CL_END_MEMORYNOTE_ADD, post.data(), NULL);
    close();
  }
}
