#ifndef CLE_MEMORY_NOTE_SUBMIT_H
#define CLE_MEMORY_NOTE_SUBMIT_H

#include <QGridLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QWidget>

extern "C" 
{
   #include "../cl_main.h"
   #include "../cl_memory.h"
   #include "../cl_network.h"
}

class CleMemoryNoteSubmit : public QWidget
{
   Q_OBJECT

public:
   CleMemoryNoteSubmit(cl_memnote_t note);
   ~CleMemoryNoteSubmit(void);

private:
   cl_memnote_t m_MemoryNote;
   
   QLineEdit   *m_Title;
   QTextEdit   *m_Description;
   QPushButton *m_SubmitButton;
   QPushButton *m_CancelButton;

private slots:
   void onClickCancel(void);
   void onClickSubmit(void);
};

#endif
