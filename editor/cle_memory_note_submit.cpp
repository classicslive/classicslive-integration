#ifndef CLE_MEMORY_NOTE_SUBMIT_CPP
#define CLE_MEMORY_NOTE_SUBMIT_CPP

#include "cle_memory_note_submit.h"

CleMemoryNoteSubmit::CleMemoryNoteSubmit(cl_memnote_t note)
{
   QGridLayout *Layout = new QGridLayout;
   m_MemoryNote = note;

   m_CancelButton = new QPushButton(tr("Cancel"));
   connect(m_CancelButton, SIGNAL(clicked()), this, SLOT(onClickCancel()));
   m_SubmitButton = new QPushButton(tr("Submit"));
   connect(m_SubmitButton, SIGNAL(clicked()), this, SLOT(onClickSubmit()));

   m_Title = new QLineEdit;
   m_Title->setPlaceholderText(tr("Title"));

   m_Description = new QTextEdit;
   m_Description->setPlaceholderText(tr("Description (optional)"));

   Layout->addWidget(m_Title,        0, 0, 1, 2);
   Layout->addWidget(m_Description,  1, 0, 1, 2);
   Layout->addWidget(m_CancelButton, 2, 0, 1, 1);
   Layout->addWidget(m_SubmitButton, 2, 1, 1, 1);

   setLayout(Layout);
   setWindowFlags(Qt::WindowTitleHint);
   setWindowTitle(tr("Create new memory note"));
}

CleMemoryNoteSubmit::~CleMemoryNoteSubmit(void)
{
   cl_free_memnote(&m_MemoryNote);
}

void CleMemoryNoteSubmit::onClickCancel(void)
{
   delete this;
}

void CleMemoryNoteSubmit::onClickSubmit(void)
{
   if (m_Title->text().isEmpty())
      return;
   else
   {
      char post_data[2048];
      const char *description = m_Description->toPlainText().toLocal8Bit().data();
      const char *title = m_Title->text().toLocal8Bit().data();

      snprintf(post_data, sizeof(post_data), 
         "game_id=%u&address=%u&type=%u&title=%s&description=%s",
         session.game_id,
         m_MemoryNote.address,
         m_MemoryNote.type,
         title,
         description);
      cl_network_post(CL_REQUEST_ADD_MEMNOTE, post_data, NULL, NULL);

      delete this;
   }
}

#endif
