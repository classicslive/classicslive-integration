#ifndef CLE_HEX_VIEW_CPP
#define CLE_HEX_VIEW_CPP

#include <string>

#include <QBrush>
#include <QPainter>
#include <QPaintEvent>
#include <QPen>
#include <QInputDialog>
#include <QRect>
#include <QWidget>

#include "cle_hex_view.h"

void swap_rects(QRect *a, QRect *b)
{
   QRect temp = *a;

   *a = *b;
   *b = temp;
} 

CleHexWidget::CleHexWidget(QWidget *parent, uint8_t size) : QWidget(parent)
{
   uint32_t i;

   for (i = 0; i < 16; i++)
   {
      m_AddrRects[i].setSize(QSize(64, 16));
      m_AddrRects[i].moveTo(0, i * 16);
   }
   m_Pixmap = QPixmap(sizeHint());

   m_Font = QFont("Monospace");
   m_Font.setPixelSize(12);
   m_Font.setStyleHint(QFont::TypeWriter);

   m_Painter = new QPainter(&m_Pixmap);
   m_Painter->setFont(m_Font);

   setBackgroundRole(QPalette::Base);
   setFocusPolicy(Qt::ClickFocus);

   setByteSwapEnabled(false);
   setOffset(0);
   setSize(size);

   repaintAll();
}

void CleHexWidget::keyPressEvent(QKeyEvent *event)
{
   switch (event->key())
   {
   case Qt::Key_Left:
      if (m_CursorOffset > 0)
         setCursorOffset(m_CursorOffset - 1);
      break;
   case Qt::Key_Right:
      if (m_CursorOffset < 256)
         setCursorOffset(m_CursorOffset + 1);
      break;
   case Qt::Key_Up:
      if (m_CursorOffset > 16)
         setCursorOffset(m_CursorOffset - 16);
      else if (event->modifiers() & Qt::ShiftModifier)
         emit offsetEdited(-0x100);
      else
         emit offsetEdited(-0x10);
      break;
   case Qt::Key_Down:
      if (m_CursorOffset < 240)
         setCursorOffset(m_CursorOffset + 16);
      else if (event->modifiers() & Qt::ShiftModifier)
         emit offsetEdited(0x100);
      else
         emit offsetEdited(0x10);
      break;
   case Qt::Key_0:
   case Qt::Key_1:
   case Qt::Key_2:
   case Qt::Key_3:
   case Qt::Key_4:
   case Qt::Key_5:
   case Qt::Key_6:
   case Qt::Key_7:
   case Qt::Key_8:
   case Qt::Key_9:
      typeNybble(event->key() - Qt::Key_0);
      break;
   case Qt::Key_A:
   case Qt::Key_B:
   case Qt::Key_C:
   case Qt::Key_D:
   case Qt::Key_E:
   case Qt::Key_F:
      typeNybble(event->key() - Qt::Key_A + 10);
      break;
   /* N64 endianness quirk */
   case Qt::Key_N:
   {
      uint32_t i;

      for (i = 0; i < 256; i += 4)
      {
         swap_rects(&m_AsciiRects[i],   &m_AsciiRects[i+3]);
         swap_rects(&m_AsciiRects[i+1], &m_AsciiRects[i+2]);
      }
      repaintAll();
      break;
   }
   case Qt::Key_S:
      setByteSwapEnabled(!m_UseByteSwap);
      break;
   case Qt::Key_G:
      {
         uint32_t address;
         QString  address_text;
         bool     ok;

         address_text = QInputDialog::getText(this, 
            tr("Goto Address"), tr("Address to goto:"));
         address = address_text.toULong(&ok, 16);
         if (ok)
         {
            emit offsetEdited(address - m_Offset);
            setCursorOffset(address & 0x0F);
         }
      }
   default:
      return;
   }
   event->accept();
}

void CleHexWidget::mousePressEvent(QMouseEvent *event)
{
   if (m_Size != 1)
      return; //todo
   else
   {
      uint32_t i;

      for (i = 0; i < 256; i++)
      {
         if (m_Rects[i].contains(event->pos()))
         {
            if (event->button() == Qt::LeftButton)
               setCursorOffset(i);
            else if (event->button() == Qt::RightButton)
               emit onRightClick(m_Offset + i);
               
            return;
         }
      }
   }
}

void CleHexWidget::paintEvent(QPaintEvent *event)
{
   QPainter painter(this);
   painter.drawPixmap(0, 0, m_Pixmap);
}

void CleHexWidget::repaintAll()
{
   uint32_t i;

   /* Draw the navigation bar on the left */
   m_Painter->setBrush(Qt::NoBrush);
   m_Painter->setPen(QColor("grey"));
   for (i = 0; i < 16; i++)
      m_Painter->drawText(m_AddrRects[i], Qt::AlignRight | Qt::AlignVCenter, m_AddrTexts[i]);

   /* Draw the hex value view on the right */
   for (i = 0; i < 256 / m_Size; i++)
   {
      /* Draw rects */
      m_Painter->setBrush(m_RectColors[i]);
      m_Painter->setPen(Qt::NoPen);
      m_Painter->drawRect(m_Rects[i]);

      /* Draw text */
      m_Painter->setPen(QColor("white"));
      m_Painter->drawText(m_Rects[i], Qt::AlignCenter, m_Texts[i]);
   }

   /* Draw the ASCII display */
   m_Painter->setPen(QColor("grey"));
   for (i = 0; i < 256; i++)
      m_Painter->drawText(m_AsciiRects[i], Qt::AlignCenter, QString(m_AsciiText[i]));

   m_Painter->drawRect(m_Cursor);
}

void CleHexWidget::repaintAscii(char new_char, uint8_t index)
{
   m_AsciiText[index] = new_char < 0x20 ? '.' : new_char;

   /* Clear area */
   m_Painter->setBrush(palette().color(backgroundRole()));
   m_Painter->setPen(Qt::NoPen);
   m_Painter->drawRect(m_AsciiRects[index]);

   /* Draw char */
   m_Painter->setFont(m_Font);
   m_Painter->setPen(QColor("grey"));
   m_Painter->drawText(m_AsciiRects[index], Qt::AlignCenter, QString(m_AsciiText[index]));
}

void CleHexWidget::repaintRect(const void *buffer, uint8_t index)
{
   /* Update the hex value drawn over the given rect */
   if (buffer)
   {
      switch (m_Size)
      {
      case 1:
         snprintf(m_Texts[index], 16, "%02X", 
            *((uint8_t*)(buffer) + index));
         break;
      case 2:
         {
            uint16_t value = *((uint16_t*)(buffer) + index);

            snprintf(m_Texts[index], 16, "%04X", 
               m_UseByteSwap ? __builtin_bswap16(value) : value);
         }
         break;
      case 4:
         {
            uint32_t value = *((uint32_t*)(buffer) + index);

            snprintf(m_Texts[index], 16, "%08X", 
               m_UseByteSwap ? __builtin_bswap32(value) : value);
         }
      }
   }

   /* Draw rect */
   m_Painter->setBrush(m_RectColors[index]);
   m_Painter->setPen(Qt::NoPen);
   m_Painter->drawRect(m_Rects[index]);

   /* Draw text */
   m_Painter->setPen(QColor("white"));
   m_Painter->drawText(m_Rects[index], Qt::AlignCenter, m_Texts[index]);
}

void CleHexWidget::resetColors()
{
   uint32_t i;

   for (i = 0; i < 256; i++)
      m_RectColors[i] = QColor(0, 0, 0);
   m_AllDirty = true;
}

void CleHexWidget::setByteSwapEnabled(bool enabled)
{
   m_UseByteSwap = enabled;
}

void CleHexWidget::setCursorOffset(uint8_t offset)
{
   /* Repaint old highlight */
   m_RectColors[m_CursorOffset].setBlue(0);
   repaintRect(NULL, m_CursorOffset);

   m_CursorNybble = 0;
   m_CursorOffset = offset;
   m_CursorValue  = 0;

   /* Repaint new highlight */
   m_RectColors[offset].setBlue(255);
   repaintRect(NULL, offset);
}

void CleHexWidget::setOffset(uint32_t offset)
{
   uint8_t i;

   m_Painter->setBrush(palette().color(backgroundRole()));
   m_Painter->setFont(m_Font);
   m_Painter->setPen(Qt::NoPen);
   m_Painter->drawRect(QRect(0, 0, 64, 256));

   for (i = 0; i < 16; i++)
   {
      snprintf(m_AddrTexts[i], 16, "%8X", offset + i * 16);
      
      m_Painter->setPen(QColor("grey"));
      m_Painter->drawText(m_AddrRects[i], Qt::AlignRight | Qt::AlignVCenter, m_AddrTexts[i]);
   }
   resetColors();

   m_Offset = offset;
}

void CleHexWidget::setSize(uint8_t size)
{
   uint32_t i, j;

   m_Size = size;
   for (i = 0, j = 0; i < 256; i += size, j++)
   {
      m_Rects[j].setSize(QSize(24 * size, 16));
      m_Rects[j].moveTo(64 + ((j * 24 * size) % 384), (i / 16) * 16);
   }
   for (i = 0; i < 256; i++)
   {
      m_AsciiRects[i].setSize(QSize(8, 16));
      m_AsciiRects[i].moveTo(64 + 384 + ((i * 8) % 128), (i / 16) * 16);
   }
   resetColors();
}

QSize CleHexWidget::sizeHint() const
{
   return QSize(64 + (24 * 16) + (8 * 16), 256);
}

void CleHexWidget::typeNybble(uint8_t value)
{
   /* TODO: Discern which byte of a (half)word just got edited here */
   if (m_CursorNybble % 2 == 0)
      value <<= 4;
   m_CursorNybble++;
   m_CursorValue += value;

   if (m_CursorNybble >= m_Size * 2)
   {
      emit valueEdited(m_Offset + m_CursorOffset, m_CursorValue);
      setCursorOffset(m_CursorOffset + 1);
   }
}

void CleHexWidget::refresh(const void *newbuffer, const void *oldbuffer)
{
   uint32_t i;

   for (i = 0; i < 256 / m_Size; i++)
   {
      /* This value has changed, paint its background red */
      if (memcmp(((uint8_t*)newbuffer) + i * m_Size, ((uint8_t*)oldbuffer) + i * m_Size, m_Size))
         m_RectColors[i].setRed(240);
      /* This value hasn't changed, fade the red out */
      else if (m_RectColors[i].red() > 0)
         m_RectColors[i].setRed(m_RectColors[i].red() - 16);
      else if (!m_AllDirty)
         continue;

      repaintRect(newbuffer, i);
   }
   for (i = 0; i < 256; i++)
      if (((char*)newbuffer)[i] != ((char*)oldbuffer)[i] || m_AllDirty)
         repaintAscii(((char*)newbuffer)[i], i);
   m_AllDirty = false;

   update();
}

void CleHexWidget::wheelEvent(QWheelEvent* event)
{
   /* Scroll one row per 15 degree increment, 10 rows if holding Shift */
   int16_t amount = -0x10;
   int16_t steps = event->angleDelta().y() / 8 / 15;

   amount *= event->modifiers() & Qt::ShiftModifier   ? 0x10 : 1;
   amount *= event->modifiers() & Qt::ControlModifier ? 0x10 : 1;
   emit offsetEdited(amount * steps);
   event->accept();
}

#endif
