#include <string>

#include <QBrush>
#include <QEvent>
#include <QMenu>
#include <QPainter>
#include <QPaintEvent>
#include <QPen>
#include <QInputDialog>
#include <QRect>
#include <QWidget>

extern "C"
{
  #include "../cl_memory.h"
}

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
   m_Image = QImage(sizeHint(), QImage::Format_RGB16);

   m_Font = QFont("Courier");
   m_Font.setPixelSize(12);
   m_Font.setStyleHint(QFont::TypeWriter);

   m_Painter = new QPainter(&m_Image);
   m_Painter->setFont(m_Font);

   setBackgroundRole(QPalette::Base);
   //setContextMenuPolicy(Qt::CustomContextMenu);
   setFocusPolicy(Qt::ClickFocus);

   setByteSwapEnabled(false);
   setOffset(0);
   setSize(size);

   repaintAll();
}

void CleHexWidget::changeEvent(QEvent *event)
{
   if (event->type() == QEvent::PaletteChange)
   {
      /* Re-draw the address sidebar with the updated background color */
      m_Painter->setBrush(palette().color(backgroundRole()));
      m_Painter->setPen(Qt::NoPen);
      m_Painter->drawRect(QRect(0, 0, 64, 256));
      m_Painter->setPen(QColor("grey"));
      for (uint8_t i = 0; i < 16; i++)
         m_Painter->drawText(m_AddrRects[i], Qt::AlignRight | Qt::AlignVCenter, m_AddrTexts[i]);
      update();
   }
   QWidget::changeEvent(event);
}

void CleHexWidget::keyPressEvent(QKeyEvent *event)
{
   switch (event->key())
   {
   case Qt::Key_Left:
      if (m_CursorNybble > 0)
      {
         /* Move back one nybble within the current cell */
         m_CursorNybble--;
         paintCursorCell(((m_CursorOffset - m_Position) & 0xFF) / m_Size);
         update();
      }
      else
      {
         /* Cross to the last nybble of the previous cell */
         if (isCursorTopLeft())
            movePosition(-0x10);
         setCursorOffset(m_CursorOffset - m_Size);
         m_CursorNybble = m_Size * 2 - 1;
         paintCursorCell(((m_CursorOffset - m_Position) & 0xFF) / m_Size);
         update();
      }
      break;
   case Qt::Key_Right:
      if (m_CursorNybble < m_Size * 2 - 1)
      {
         /* Move forward one nybble within the current cell */
         m_CursorNybble++;
         paintCursorCell(((m_CursorOffset - m_Position) & 0xFF) / m_Size);
         update();
      }
      else
      {
         /* Cross to the first nybble of the next cell */
         if (isCursorBottomRight())
            movePosition(0x10);
         setCursorOffset(m_CursorOffset + m_Size);
      }
      break;
   case Qt::Key_Return:
   case Qt::Key_Enter:
      emit valueEdited(m_CursorOffset, m_CursorValue, m_Size);
      if (isCursorBottomRight())
         movePosition(0x10);
      setCursorOffset(m_CursorOffset + m_Size);
      break;
   case Qt::Key_Up:
      if (event->modifiers() & Qt::ShiftModifier)
      {
         movePosition(-0x100);
         setCursorOffset(m_CursorOffset - 0x100);
      }
      else
      {
         if (isCursorTop())
            movePosition(-0x10);
         setCursorOffset(m_CursorOffset - 0x10);
      }
      break;
   case Qt::Key_Down:
      if (event->modifiers() & Qt::ShiftModifier)
      {
         movePosition(0x100);
         setCursorOffset(m_CursorOffset + 0x100);
      }
      else
      {
         if (isCursorBottom())
            movePosition(0x10);
         setCursorOffset(m_CursorOffset + 0x10);
      }
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
            setOffset(address);
            setCursorOffset(address);
         }
      }
   default:
      return;
   }
   event->accept();
}

void CleHexWidget::mousePressEvent(QMouseEvent *event)
{
   QPoint pos = event->pos();
   uint32_t i;
   uint32_t cell_count = 256 / m_Size;

   for (i = 0; i < cell_count; i++)
   {
      if (m_Rects[i].contains(pos))
      {
         cl_addr_t address = m_Position + (cl_addr_t)(i * m_Size);
         if (event->button() == Qt::LeftButton)
            setCursorOffset(address);
         else if (event->button() == Qt::RightButton)
            onRightClick(address, pos);
         return;
      }
   }
}

void CleHexWidget::movePosition(int32_t offset)
{
   if (offset < 0)
   {
      cl_addr_t delta = static_cast<cl_addr_t>(-offset);
      if (delta > m_Position - m_PositionMin)
         setOffset(m_PositionMin);
      else
         setOffset(m_Position - delta);
   }
   else
   {
      cl_addr_t new_pos = m_Position + static_cast<cl_addr_t>(offset);
      if (new_pos + 0x100 >= m_PositionMax)
         setOffset(m_PositionMax - 0x100);
      else
         setOffset(new_pos);
   }
}

void CleHexWidget::onClickAddMemoryNote()
{
   emit requestAddMemoryNote(m_CursorOffset);
}

void CleHexWidget::onClickGoto()
{
   setOffset(m_CursorOffset);
}

void CleHexWidget::onClickPointerSearch()
{
   emit requestPointerSearch(m_CursorOffset);
}

void CleHexWidget::onRightClick(cl_addr_t address, QPoint& pos)
{
  if (address < m_Position || pos.isNull())
    return;
  else
  {
    QMenu menu;
    QAction *action_add = menu.addAction(tr("&Add memory note..."));
    QAction *action_ptr = menu.addAction(tr("Search for &pointers"));
    QAction *action_goto;
    const cl_memory_region_t *region;
    cl_addr_t goto_address;
    cl_value_type ptr_type;

    setCursorOffset(address);
    connect(action_add, SIGNAL(triggered()), this,
      SLOT(onClickAddMemoryNote()));
    connect(action_ptr, SIGNAL(triggered()), this,
      SLOT(onClickPointerSearch()));

    /* Allow following a pointer if it is valid */
    region = cl_find_memory_region(address);
    if (!region)
      return;

    ptr_type = cl_pointer_type(region->pointer_length);
    if (cl_read_memory_value(&goto_address, nullptr, address, ptr_type) &&
      cl_find_memory_region(goto_address))
    {
      m_CursorOffset = goto_address;
      action_goto = menu.addAction(tr("&Goto this address"));
      connect(action_goto, SIGNAL(triggered()), this,
        SLOT(onClickGoto()));
    }

    menu.exec(mapToGlobal(pos));
  }
}

void CleHexWidget::paintEvent(QPaintEvent *event)
{
   QPainter painter(this);
   painter.drawImage(0, 0, m_Image);
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
  int64_t val = 0;

  /* Update the hex value drawn over the given rect */
  if (buffer)
  {
    cl_read_value(&val,
                  reinterpret_cast<const uint8_t*>(buffer),
                  index * m_Size,
                  cl_pointer_type(m_Size),
                  m_UseByteSwap ? CL_ENDIAN_BIG : CL_ENDIAN_LITTLE);

    switch (m_Size)
    {
    case 1:
      snprintf(m_Texts[index], 16, "%02X", val);
      break;
    case 2:
      snprintf(m_Texts[index], 16, "%04X", val);
      break;
    case 4:
      snprintf(m_Texts[index], 16, "%08X", val);
      break;
    case 8:
      snprintf(m_Texts[index], 16, "%016llX", val);
      break;
    default:
      snprintf(m_Texts[index], 16, "Err");
    }

    /* Draw rect */
    m_Painter->setBrush(m_RectColors[index]);
    m_Painter->setPen(Qt::NoPen);
    m_Painter->drawRect(m_Rects[index]);

    /* Draw text */
    if (m_PositionMin && m_Size == 4 && (val >= m_PositionMin && val < m_PositionMax))
      /* Draw likely pointers in a different color */
      m_Painter->setPen(QColor(200, 180, 255));
    else
      m_Painter->setPen(QColor(255, 255, 255));
    m_Painter->drawText(m_Rects[index], Qt::AlignCenter, m_Texts[index]);
  }
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

void CleHexWidget::paintCursorCell(uint8_t index)
{
   m_Painter->setFont(m_Font);
   m_Painter->setBrush(m_RectColors[index]);
   m_Painter->setPen(Qt::NoPen);
   m_Painter->drawRect(m_Rects[index]);
   m_Painter->setPen(QColor("white"));
   m_Painter->drawText(m_Rects[index], Qt::AlignCenter, m_Texts[index]);

   /* Draw nybble-position underline on the active cursor cell */
   if (m_RectColors[index].blue() > 0)
   {
      uint8_t total_nybbles = m_Size * 2;
      uint8_t nybble = (m_CursorNybble < total_nybbles) ? m_CursorNybble : total_nybbles - 1;

      QFontMetrics fm(m_Font);
      int char_w   = fm.horizontalAdvance(QLatin1Char('F'));
      int text_w   = char_w * total_nybbles;
      int text_x   = m_Rects[index].x() + (m_Rects[index].width() - text_w) / 2;
      int line_x   = text_x + nybble * char_w;
      int line_y   = m_Rects[index].bottom() - 1;

      m_Painter->setPen(QPen(QColor("white"), 2));
      m_Painter->drawLine(line_x, line_y, line_x + char_w - 1, line_y);
   }
}

void CleHexWidget::setCursorOffset(cl_addr_t offset)
{
   uint8_t cursor = ((m_CursorOffset - m_Position) & 0xFF) / m_Size;

   /* Clear old highlight directly into m_Image */
   m_RectColors[cursor].setBlue(0);
   paintCursorCell(cursor);

   m_CursorOffsetBase = offset; /* save before alignment for size changes */

   /* Align to value size boundary */
   offset &= ~(cl_addr_t)(m_Size - 1);

   /* Check bounds */
   if (offset < m_PositionMin)
      m_CursorOffset = m_PositionMin;
   else if (offset >= m_PositionMax)
      m_CursorOffset = m_PositionMax;
   else
      m_CursorOffset = offset;

   /* Set new highlight directly into m_Image */
   cursor = ((m_CursorOffset - m_Position) & 0xFF) / m_Size;
   m_CursorNybble = 0;
   m_CursorValue  = 0;
   sscanf(m_Texts[cursor], "%llX", (unsigned long long*)&m_CursorValue);
   m_RectColors[cursor].setBlue(255);
   paintCursorCell(cursor);

   update();
}

void CleHexWidget::setOffset(cl_addr_t offset)
{
  uint8_t i;

  /* Only deal with rows of 16 bytes */
  offset &= ~0xF;
  if (offset == m_Position)
    return;

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
  m_Position = offset;
  setCursorOffset(m_CursorOffset);

  emit offsetEdited(offset);
}

void CleHexWidget::setRange(cl_addr_t min, cl_addr_t max)
{
   m_PositionMin = min;
   m_PositionMax = max;
   setOffset(min);
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

   /* Re-align cursor from the saved byte-level base to the new size boundary.
      This preserves position when sizing down and snaps to alignment when sizing up. */
   m_CursorOffset = m_CursorOffsetBase & ~(cl_addr_t)(size - 1);
   if (m_PositionMin != (cl_addr_t)-1 && m_CursorOffset < m_PositionMin)
      m_CursorOffset = m_PositionMin & ~(cl_addr_t)(size - 1);
   else if (m_PositionMax != (cl_addr_t)-1 && m_CursorOffset >= m_PositionMax)
      m_CursorOffset = (m_PositionMax - size) & ~(cl_addr_t)(size - 1);

   uint8_t cursor = ((m_CursorOffset - m_Position) & 0xFF) / size;
   m_RectColors[cursor].setBlue(255);
   paintCursorCell(cursor);
   update();
}

QSize CleHexWidget::sizeHint() const
{
   return QSize(64 + (24 * 16) + (8 * 16), 256);
}

void CleHexWidget::typeNybble(uint8_t value)
{
   uint8_t total_nybbles = m_Size * 2;
   uint8_t shift = (total_nybbles - 1 - m_CursorNybble) * 4;

   m_CursorValue &= ~((uint64_t)0xF << shift);
   m_CursorValue |= (uint64_t)value << shift;
   m_CursorNybble++;

   /* Update cell text to show the partial value being typed */
   uint8_t cursor = ((m_CursorOffset - m_Position) & 0xFF) / m_Size;
   switch (m_Size)
   {
   case 1: snprintf(m_Texts[cursor], 16, "%02X",              (unsigned)m_CursorValue);              break;
   case 2: snprintf(m_Texts[cursor], 16, "%04X",              (unsigned)m_CursorValue);              break;
   case 4: snprintf(m_Texts[cursor], 16, "%08X",              (unsigned)m_CursorValue);              break;
   case 8: snprintf(m_Texts[cursor], 16, "%016llX", (unsigned long long)m_CursorValue);              break;
   }
   paintCursorCell(cursor);
   update();

   if (m_CursorNybble >= total_nybbles)
   {
      emit valueEdited(m_CursorOffset, m_CursorValue, m_Size);
      setCursorOffset(m_CursorOffset + m_Size);
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
   movePosition(amount * steps);
   event->accept();
}
