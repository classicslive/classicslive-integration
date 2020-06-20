#ifndef CLE_RESULT_TABLE_CPP
#define CLE_RESULT_TABLE_CPP

#include <QHeaderView>

#include "cle_result_table.h"

QTableWidget* CleResultTable::getTable()
{
   return m_Table;
}

void CleResultTable::init()
{
   /* Initialize basic table properties and style */
   m_Table = new QTableWidget();
   m_Table->setRowCount(0);
   m_Table->setContextMenuPolicy(Qt::CustomContextMenu);
   m_Table->setAlternatingRowColors(true);
   m_Table->setShowGrid(false);
   m_Table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::Stretch);
   m_Table->verticalHeader()->setVisible(false);
   m_Table->verticalHeader()->setDefaultSectionSize(16);

   /* Setup Qt slots */
   connect(m_Table, SIGNAL(itemClicked(QTableWidgetItem*)),            this, SLOT(onResultClick()));
   connect(m_Table, SIGNAL(itemDoubleClicked(QTableWidgetItem*)),      this, SLOT(onResultDoubleClick()));
   connect(m_Table, SIGNAL(itemChanged(QTableWidgetItem*)),            this, SLOT(onResultEdited(QTableWidgetItem*)));
   connect(m_Table, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(onResultRightClick(const QPoint&)));
   connect(m_Table, SIGNAL(itemSelectionChanged()),                    this, SLOT(onResultSelectionChanged()));

   m_ClickedResult = -1;
   m_CurrentEditedRow = -1;
}

#endif
