#ifndef CLE_MEMORY_INSPECTOR_H
#define CLE_MEMORY_INSPECTOR_H

#include <QComboBox>
#include <QGridLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QScrollBar>
#include <QTabBar>
#include <QTableWidget>
#include <QTimer>
#include <QWidget>

extern "C" 
{
   #include "../cl_main.h"
   #include "../cl_memory.h"
   #include "../cl_network.h"
   #include "../cl_search.h"
}
#include "cle_hex_view.h"
#include "cle_memory_note_submit.h"

/* TODO: Move this to a config option? */
#define CLE_SEARCH_MAX_ROWS 1000

#define CLE_SEARCHTYPE_NORMAL  0
#define CLE_SEARCHTYPE_POINTER 1

class CleMemoryInspector : public QWidget
{
   Q_OBJECT

public:
   CleMemoryInspector(cl_memory_t *memory);

public slots:
   void update();

private:
   cl_memory_t       *m_Memory;
   uint8_t            m_SearchTypes  [8];
   cl_search_t        m_Searches     [8];
   cl_pointersearch_t m_PointerSearch[8];

   CleHexWidget        *m_HexWidget;
   CleMemoryNoteSubmit *m_MemoryNoteSubmit;

   uint32_t      m_AddressOffset;
   uint8_t      *m_BufferPrevious;
   uint8_t      *m_BufferCurrent;
   int32_t       m_ClickedResult;
   int8_t        m_ClickedTab;
   int32_t       m_CurrentEditedRow;
   uint8_t       m_CurrentTab;
   uint8_t       m_TabCount;
   
   QComboBox    *m_CompareDropdown;
   QComboBox    *m_SizeDropdown;
   QLineEdit    *m_TextEntry;
   QPushButton  *m_NewButton;
   QPushButton  *m_SearchButton;
   QTabBar      *m_Tabs;
   QTableWidget *m_ResultTable;
   QTimer       *m_UpdateTimer;

   uint8_t  getCurrentSizeType(void);
   uint32_t getClickedResultAddress(void);
   void     rebuildRows(void);
   void     rebuildRowsNormal(void);
   void     rebuildRowsPointer(void);
   void     updateNormal(void);
   void     updatePointer(void);

private slots:
   void onChangeCompareType();
   void onChangeSizeType();
   void onChangeTab();
   void onClickNew();
   void onClickResultAddMemoryNote();
   void onClickResultPointerSearch();
   void onClickResultRemove();
   void onClickSearch();
   void onClickTabRename();

   void onHexWidgetOffsetEdited(int32_t delta);
   void onHexWidgetValueEdited(uint32_t address, uint8_t value);

   void onResultClicked();
   void onResultDoubleClicked();
   void onResultEdited(QTableWidgetItem *result);
   void onResultSelectionChanged();

   void onRightClickResult(const QPoint &pos);
   void onRightClickTabs(const QPoint &pos);
};

#endif
