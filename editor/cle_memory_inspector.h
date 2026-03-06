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
   CleResultTable *m_CurrentSearch = nullptr;

   CleHexWidget        *m_HexWidget = nullptr;
   CleMemoryNoteSubmit *m_MemoryNoteSubmit = nullptr;

   cl_memory_region_t *m_CurrentMembank = nullptr;

   cl_addr_t m_AddressOffset;
   cl_addr_t m_CurrentRegionSize = 0;
   unsigned  m_CurrentRegionCount = 0;
   uint8_t  *m_BufferPrevious = nullptr;
   uint8_t  *m_BufferCurrent = nullptr;
   int8_t    m_ClickedTab;
   uint8_t   m_TabCount;

   QComboBox      *m_CompareDropdown = nullptr;
   QGridLayout    *m_Layout = nullptr;
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
   void    applyTheme(QWidget *w);
   void    rebuildLayout(void);

private slots:
   void onAddressChanged(cl_addr_t address);
   void onChangeCompareType();
   void onChangeRegionTab(int index);
   void onChangeScrollbar(int value);
   void onChangeSizeType();
   void onChangeTab();
   void onClickNew();
   void onClickSearch();
   void onClickTabRename();

   void onHexWidgetValueEdited(cl_addr_t address, uint64_t value, uint8_t size);

   void onRightClickTabs(const QPoint &pos);

   void onTargetChanged(const QString&);

   void requestAddMemoryNote(cl_memnote_t note);
   void requestAddMemoryNote(cl_addr_t address);
   void requestPointerSearch(cl_addr_t address);
};

#endif
