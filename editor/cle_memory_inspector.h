#ifndef CLE_MEMORY_INSPECTOR_H
#define CLE_MEMORY_INSPECTOR_H

#include <QComboBox>
#include <QGridLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
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
#include "cle_result_table.h"

class CleMemoryInspector : public QWidget
{
   Q_OBJECT

public:
   CleMemoryInspector();

public slots:
   void update();

private:
   CleResultTable *m_Searches[8];
   CleResultTable *m_CurrentSearch;

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
   QTimer       *m_UpdateTimer;

   uint8_t  getCurrentCompareType(void);
   uint8_t  getCurrentSizeType(void);
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
   void onHexWidgetRightClick(uint32_t address);
   void onHexWidgetValueEdited(uint32_t address, uint8_t value);

   void onResultClicked();
   void onResultDoubleClicked();
   void onResultEdited(QTableWidgetItem *result);
   void onResultSelectionChanged();

   void onRightClickResult(const QPoint &pos);
   void onRightClickTabs(const QPoint &pos);
};

#endif
