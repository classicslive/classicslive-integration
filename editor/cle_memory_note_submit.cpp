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
   snprintf(footer_text, sizeof(footer_text), "0x%08X - %u pointer passes",
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
      char post_data[2048];
      char pointer_data[256] = {0};
      std::string stdtitle = m_Title->text().toStdString();
      std::string stddescription = m_Description->toPlainText().toStdString();
      const char *title = stdtitle.c_str();
      const char *description = stddescription.c_str();

      /* Implode offset array into space-separated string */
      if (m_MemoryNote.pointer_passes)
      {
         QString passes;
         uint8_t i;

         for (i = 0; i < m_MemoryNote.pointer_passes; i++)
           passes += QString::number(m_MemoryNote.pointer_offsets[i]) + " ";
         strncpy(pointer_data, passes.toStdString().c_str(), passes.length() - 1);
      }

      snprintf(post_data, sizeof(post_data), 
         "game_id=%u&address=%u&type=%u&offsets=%s&title=%s%s%s",
         session.game_id,
         m_MemoryNote.address,
         m_MemoryNote.type,
         pointer_data,
         title,
         strlen(description) ? "&description=" : "",
         strlen(description) ? description : "");
      cl_network_post(CL_REQUEST_ADD_MEMNOTE, post_data, NULL);

      close();
   }
}
