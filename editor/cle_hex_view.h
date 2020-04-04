#ifndef CLE_HEX_VIEW_H
#define CLE_HEX_VIEW_H

#include <stdint.h>

#include <QWidget>

class CleHexWidget : public QWidget
{
   Q_OBJECT

public:
   CleHexWidget(QWidget *parent, uint8_t size);

   void  refresh(const void *newbuffer, const void *oldbuffer);
   void  setByteSwapEnabled(bool enabled);
   void  setOffset(uint32_t offset);
   void  setSize(uint8_t size);

   QSize sizeHint() const override;

signals:
   void offsetEdited(int32_t delta);
   void valueEdited(uint32_t address, uint8_t value);

protected:
   void keyPressEvent(QKeyEvent *event) override;
   void mousePressEvent(QMouseEvent *event) override;
   void paintEvent(QPaintEvent *event) override;
   void wheelEvent(QWheelEvent* event) override;

private:
   void repaintAll();
   void repaintAscii(const char new_char, uint8_t index);
   void repaintRect(const void *buffer, uint8_t index);

   void resetColors();
   void setCursorOffset(uint8_t offset);
   void typeNybble(uint8_t value);

   QRect   m_Rects[256];
   QColor  m_RectColors[256];
   uint8_t m_Size;
   char    m_Texts[256][16];

   QRect   m_AsciiRects[256];
   char    m_AsciiText[256];

   QRect    m_AddrRects[256];
   char     m_AddrTexts[16][16];

   QFont    m_Font;
   QPixmap  m_Pixmap;

   bool     m_AllDirty;
   uint32_t m_Offset;
   bool     m_UseByteSwap;

   QRect    m_Cursor;
   uint8_t  m_CursorNybble;
   uint8_t  m_CursorOffset;
   uint32_t m_CursorValue;
};

#endif
