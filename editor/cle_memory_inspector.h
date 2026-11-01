#ifndef CLE_MEMORY_INSPECTOR_H
#define CLE_MEMORY_INSPECTOR_H

#include <vector>

#include <QSlider>
#include <QComboBox>
#include <QGridLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
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
   #include "../cl_search_pointer_new.h"
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
   void bringToFront();
   void addressChanged(cl_addr_t address);

private:
   CleResultTable *m_Searches[CLE_MAX_TABS];
   CleResultTable *m_CurrentSearch = nullptr;

   CleHexWidget        *m_HexWidget = nullptr;
   CleMemoryNoteSubmit *m_MemoryNoteSubmit = nullptr;

   cl_memory_region_t *m_CurrentMembank = nullptr;

   cl_addr_t m_AddressOffset;

   /* Where the hex view was parked in each region, indexed like memory.regions */
   std::vector<cl_addr_t> m_RegionOffsets;

   /* Where the hex view was left in each search tab */
   struct CleTabPosition
   {
     cl_addr_t view;
     cl_addr_t cursor;
     bool      valid;
   };
   CleTabPosition m_TabPositions[CLE_MAX_TABS];

   /* The tab the hex view is currently showing, or -1 if it is not tracked */
   int8_t m_ActiveTab = -1;

   cl_addr_t m_CurrentRegionSize = 0;
   unsigned  m_CurrentRegionCount = 0;
   uint8_t  *m_BufferPrevious = nullptr;
   uint8_t  *m_BufferCurrent = nullptr;
   int8_t    m_ClickedTab;
   uint8_t   m_TabCount;

   QComboBox      *m_CompareDropdown = nullptr;
   QGridLayout    *m_Layout = nullptr;
   QMenuBar       *m_MenuBar = nullptr;
   QTabBar        *m_RegionTabs = nullptr;
   QComboBox      *m_SizeDropdown = nullptr;
   QLineEdit      *m_TextEntry = nullptr;
   QPushButton    *m_NewButton = nullptr;
   QPushButton    *m_SearchButton = nullptr;
   QSlider        *m_Slider = nullptr;
   QLabel         *m_Status = nullptr;
   QStackedWidget *m_TableStack = nullptr;
   QTabBar        *m_Tabs = nullptr;
   QTimer         *m_UpdateTimer = nullptr;

   cl_compare_type getCurrentCompareType(void);
   cl_value_type getCurrentSizeType(void);
   int     regionIndex(const cl_memory_region_t *region) const;
   void    saveRegionOffset(void);
   void    saveTabPosition(void);
   void    updateNewTabButton(void);
   void    applyTheme(QWidget *w);
   void    rebuildLayout(void);

private slots:
   void changeCompareType();
   void changeRegionTab(int index);
   void changeScrollbar(int value);
   void changeSizeType();
   void changeTab();
   void clickNew();
   void clickSearch();
   void clickTabDelete();
   void clickTabRename();

   void focusedNoteChanged(cl_memnote_t note);
   void hexWidgetValueEdited(cl_addr_t address, uint64_t value, uint8_t size);

   void rightClickTabs(const QPoint &pos);

   void targetChanged(const QString&);

   void requestAddMemoryNote(cl_memnote_t note);
   void requestPointerSearch(cl_addr_t address);
   void showActiveRegions();
};

#endif
