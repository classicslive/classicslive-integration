#ifndef CLE_SCRIPT_EDITOR_CPP
#define CLE_SCRIPT_EDITOR_CPP

#include <QGridLayout>
#include <QHeaderView>

extern "C" 
{
   #include "../cl_script.h"
}
#include "cle_script_editor.h"

CleScriptEditor::CleScriptEditor()
{
   /* Initialize basic table properties and style */
   m_Table = new QTableWidget();
   m_Table->setContextMenuPolicy(Qt::CustomContextMenu);
   m_Table->setAlternatingRowColors(true);
   m_Table->setShowGrid(false);
   m_Table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::Stretch);
   m_Table->verticalHeader()->setVisible(false);
   m_Table->verticalHeader()->setDefaultSectionSize(16);

   m_Table->setColumnCount(3);

   /* Initialize result table column headers */
   QStringList TableHeader;
   TableHeader << tr("Action") << tr("Arguments") << tr("Hits");
   m_Table->setHorizontalHeaderLabels(TableHeader);

   m_UpdateTimer = new QTimer(this);
   connect(m_UpdateTimer, SIGNAL(timeout()), this, SLOT(run()));
   m_UpdateTimer->start(100);

   m_Label = new QLabel(this);

   /* Initialize window layout */
   QGridLayout* Layout = new QGridLayout(this);
   Layout->addWidget(m_Table, 0, 0);
   Layout->addWidget(m_Label, 1, 0);
   setLayout(Layout);
}

void CleScriptEditor::run()
{
   QTableWidgetItem  *item;
   const cl_action_t *action;
   char     temp_string[32];
   uint32_t i;

   if (!script.page_count)
      return;

   m_Table->setRowCount(script.pages[0].action_count);
   for (i = 0; i < m_Table->rowCount(); i++)
   {
      action = &script.pages[0].actions[i];

      /* Update action type column */
      snprintf(temp_string, sizeof(temp_string), "%c", action->type);
      m_Table->setItem(i, 0, new QTableWidgetItem(QString(temp_string)));

      m_Table->setItem(i, 1, new QTableWidgetItem(QString(temp_string)));

      /* Update arguments column */
      snprintf(temp_string, sizeof(temp_string), "%u", action->executions);
      m_Table->setItem(i, 2, new QTableWidgetItem(QString(temp_string)));
   }
   m_Label->setText("Counter 0: " + QString::number(script.pages[0].counters[0]));
}

#endif