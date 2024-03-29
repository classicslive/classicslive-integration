#ifndef CLE_HEX_VIEW_H
#define CLE_HEX_VIEW_H

#include <stdint.h>

#include <QWidget>

extern "C"
{
   #include "../cl_common.h"
}

class CleHexWidget : public QWidget
{
   Q_OBJECT

public:
   CleHexWidget(QWidget *parent, uint8_t size);

   void refresh(const void *newbuffer, const void *oldbuffer);
   void setByteSwapEnabled(bool enabled);
   void setOffset(cl_addr_t offset);
   void setRange(cl_addr_t min, cl_addr_t max);
   void setSize(uint8_t size);

   QSize sizeHint() const override;

signals:
   void offsetEdited(cl_addr_t offset);
   void requestAddMemoryNote(cl_addr_t address);
   void requestPointerSearch(cl_addr_t address);
   void valueEdited(cl_addr_t address, uint8_t value);

protected:
   void keyPressEvent(QKeyEvent *event) override;
   void mousePressEvent(QMouseEvent *event) override;
   void paintEvent(QPaintEvent *event) override;
   void wheelEvent(QWheelEvent* event) override;

private slots:
   void onClickAddMemoryNote();
   void onClickGoto();
   void onClickPointerSearch();

private:
   bool isCursorTop()
   {
      return ((m_CursorOffset - m_Position) & 0xFF) < 0x10;
   }
   bool isCursorTopLeft()
   { 
      return ((m_CursorOffset - m_Position) & 0xFF) == 0x00;
   }
   bool isCursorBottom() 
   {
      return ((m_CursorOffset - m_Position) & 0xFF) >= 0xF0;
   }
   bool isCursorBottomRight()
   { 
      return ((m_CursorOffset - m_Position) & 0xFF) == 0xFF;
   }

   void movePosition(int32_t offset);

   void repaintAll();
   void repaintAscii(const char new_char, uint8_t index);
   void repaintRect(const void *buffer, uint8_t index);

   void resetColors();
   void setCursorOffset(cl_addr_t offset);
   void typeNybble(uint8_t value);

   void onRightClick(cl_addr_t address, QPoint& pos);

   QRect   m_Rects[256];
   QColor  m_RectColors[256];
   uint8_t m_Size;
   char    m_Texts[256][16];

   QRect   m_AsciiRects[256];
   char    m_AsciiText[256];

   QRect    m_AddrRects[256];
   char     m_AddrTexts[16][16];

   QFont    m_Font;
   QPainter *m_Painter;
   QImage   m_Image;

   bool     m_AllDirty;
   bool     m_UseByteSwap;

   /* Values that define the area being viewed. */
   cl_addr_t m_Position;
   cl_addr_t m_PositionMax;
   cl_addr_t m_PositionMin;

   QRect    m_Cursor;
   uint8_t  m_CursorNybble;
   uint32_t m_CursorOffset;
   uint32_t m_CursorValue;
};

#endif
