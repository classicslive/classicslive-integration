#include "cle_memory_note_submit.h"

extern "C"
{
  #include "../cl_json.h"
  #include "../cl_network.h"
}

static CL_NETWORK_CB(cle_memory_note_add_cb)
{
  if (!response.data || !userdata)
    return;
  else
  {
    unsigned memory_note_id;

    if (cl_json_get(&memory_note_id, response.data, "memory_note_id",
                    CL_JSON_TYPE_NUMBER, sizeof(memory_note_id)))
    {
      cl_memnote_t *note = (cl_memnote_t*)userdata;
      note->key = memory_note_id;
      cl_memory_add_note(note);
    }
    free(userdata);
  }
}

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
    note.address_initial, note.pointer_passes);
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
    cl_memnote_t *note;
    QByteArray post;

    post.append("game_id=" + QByteArray::number(session.game_id));
    post.append("&address=" + QByteArray::number((qulonglong)m_MemoryNote.address_initial));
    post.append("&type=" + QByteArray::number(m_MemoryNote.type));

    /* Set the pointer parameters */
    if (m_MemoryNote.pointer_passes)
    {
      QStringList offsets;
      for (unsigned i = 0; i < m_MemoryNote.pointer_passes; ++i)
        offsets << QString::number(m_MemoryNote.pointer_offsets[i]);
      post.append("&offsets=" + QUrl::toPercentEncoding(offsets.join(" ")));
    }

    /* Set the title */
    QString title = m_Title->text();
    snprintf(m_MemoryNote.details.title,
             sizeof(m_MemoryNote.details.title),
             "%s", title.toStdString().c_str());
    post.append("&title=" + QUrl::toPercentEncoding(title));

    /* Set the description */
    QString description = m_Description->toPlainText();
    if (!description.isEmpty())
    {
      snprintf(m_MemoryNote.details.description,
               sizeof(m_MemoryNote.details.description),
               "%s", description.toStdString().c_str());
      post.append("&description=" + QUrl::toPercentEncoding(description));
    }

    /* Create a copy of the memory note to send with the network request */
    note = new cl_memnote_t;
    *note = m_MemoryNote;

    cl_network_post_api(CL_END_MEMORYNOTE_ADD, post.data(),
                        cle_memory_note_add_cb, note);
    close();
  }
}
