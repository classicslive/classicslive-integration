#ifndef CLE_MEMORY_INSPECTOR_H
#define CLE_MEMORY_INSPECTOR_H

#include <QSlider>
#include <QComboBox>
#include <QGridLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QStackedWidget>
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

#define CLE_MAX_TABS 8

class CleMemoryInspector : public QWidget
{
   Q_OBJECT

public:
   CleMemoryInspector();

public slots:
   void run();

private:
   CleResultTable *m_Searches[CLE_MAX_TABS];
   CleResultTable *m_CurrentSearch;

   CleHexWidget        *m_HexWidget;
   CleMemoryNoteSubmit *m_MemoryNoteSubmit;

   cl_memory_region_t *m_CurrentMembank;

   uint32_t  m_AddressOffset;
   uint8_t  *m_BufferPrevious;
   uint8_t  *m_BufferCurrent;
   int8_t    m_ClickedTab;
   uint8_t   m_TabCount;
   
   QComboBox      *m_CompareDropdown;
   QGridLayout    *m_Layout;
   QComboBox      *m_SizeDropdown;
   QLineEdit      *m_TextEntry;
   QPushButton    *m_NewButton;
   QPushButton    *m_SearchButton;
   QSlider        *m_Slider;
   QStackedWidget *m_TableStack;
   QTabBar        *m_Tabs;
   QTimer         *m_UpdateTimer;

   cl_compare_type getCurrentCompareType(void);
   cl_value_type getCurrentSizeType(void);
   void    rebuildLayout(void);

private slots:
   void onAddressChanged(cl_addr_t address);
   void onChangeCompareType();
   void onChangeScrollbar(int value);
   void onChangeSizeType();
   void onChangeTab();
   void onClickNew();
   void onClickSearch();
   void onClickTabRename();

   void onHexWidgetValueEdited(cl_addr_t address, uint8_t value);

   void onRightClickTabs(const QPoint &pos);

   void requestAddMemoryNote(cl_memnote_t note);
   void requestAddMemoryNote(cl_addr_t address);
   void requestPointerSearch(cl_addr_t address);
};

#endif
