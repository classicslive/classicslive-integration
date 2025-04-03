#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>

#include "cle_action_block.h"

CleActionBlock::CleActionBlock(QWidget *parent) : QWidget(parent)
{
  m_Layout = new QHBoxLayout(this);
  m_Layout->setContentsMargins(16, 4, 4, 4);

  setGeometry(0, 0, CLE_BLOCK_WIDTH, CLE_BLOCK_HEIGHT);

  m_SnapDirection = CLE_SNAP_NONE;

  setLayout(m_Layout);
}

CleActionBlock::~CleActionBlock()
{
  detach();
}

void CleActionBlock::mouseMoveEvent(QMouseEvent *event)
{
  QPoint adjusted_pos = m_DragPos.parent +
                        (event->globalPos() - m_DragPos.global) - m_DragPos.self;

  setPosition(adjusted_pos);
  emit onDrag(this);
}

void CleActionBlock::mousePressEvent(QMouseEvent *event)
{
  if (event->button() == Qt::LeftButton)
  {
    m_DragPos.global = event->globalPos();
    m_DragPos.parent = mapToParent(event->pos());
    m_DragPos.self = event->pos();
    if (event->modifiers() & Qt::ControlModifier)
      select();
    else
      selectWithChildren();
  }
  /* Delete self if right-clicked, unless this is a bookend. */
  else if (event->button() == Qt::RightButton && !isStart() && !isEnd())
    close();
}

void CleActionBlock::mouseReleaseEvent(QMouseEvent *event)
{
  if (event->button() == Qt::LeftButton)
  {
    if (event->modifiers() & Qt::ControlModifier)
      deselect();
    else
      deselectWithChildren();
  }
}

int CleActionBlock::snapIndentation(QPoint pos)
{
  if (!m_SnapZone.contains(pos))
    return -1;
  else
    return (pos.x() - m_SnapZone.x()) / CLE_BLOCK_HEIGHT;
}

void CleActionBlock::attachTo(CleActionBlock *target, int indentation)
{
  auto x = target->pos().x() + (indentation - target->indentation()) * CLE_BLOCK_HEIGHT;
  auto y = target->pos().y() + target->height();

  setPosition(QPoint(x, y));

  auto next = target->next();
  if (next && next != this)
  {
    next->setPrev(this);
    m_Next = next;
    m_Prev = target;

    while (next && next != this)
    {
      next->setPosition(QPoint(next->x(), next->y() + next->height()));
      next = next->next();
    }
  }

  target->setNext(this);
  m_Indentation = indentation;
  m_Prev = target;
}

void CleActionBlock::detach()
{
  if (m_Prev)
    m_Prev->setNext(m_Next);
  if (m_Next)
  {
    auto next = m_Next;

    m_Next->setPrev(m_Prev);
    while (next)
    {
      next->setPosition(QPoint(next->x(), next->y() - CLE_BLOCK_HEIGHT));
      next = next->next();
    }
  }
  setPrev(nullptr);
  setNext(nullptr);
  m_Indentation = 0;
}

void CleActionBlock::paintEvent(QPaintEvent *e)
{
  QPainter painter(this);
  CL_UNUSED(e);

  /* Draw a light border around all elements */
  painter.setPen(Qt::darkGray);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.drawRoundedRect(QRectF(1, 1, CLE_BLOCK_WIDTH - 2, CLE_BLOCK_HEIGHT - 1), 2.0, 2.0);

  /* Draw a colored tab to the left of the block to represent indentation level */
  if (m_Indentation >= 0)
  {
    switch (m_Indentation % 4)
    {
    case 0:
      painter.setBrush(QColor(15, 99, 179));
      break;
    case 1:
      painter.setBrush(QColor(189, 60, 48));
      break;
    case 2:
      painter.setBrush(QColor(0, 121, 61));
      break;
    case 3:
      painter.setBrush(QColor(167, 54, 169));
      break;
    }
    painter.setPen(Qt::NoPen);
    painter.drawRect(0, 0, 4, height());
  }

  if (m_SnapDirection != CLE_SNAP_NONE)
  {
    QLinearGradient gradient(0, 0, width(), 4);
    gradient.setColorAt(0, Qt::darkBlue);
    gradient.setColorAt(1, Qt::blue);

    painter.setBrush(gradient);
    painter.setPen(Qt::NoPen);
    painter.drawRect(0, 0, width(), 4);
  }
}

void CleActionBlock::onSnap(const QPoint *position, cle_block_snap direction)
{
  m_SnapDirection = direction;
  setPosition(*position);
}

void CleActionBlock::setPosition(QPoint pos)
{
  move(pos);
  m_SnapZone = QRect(
      pos.x() - CLE_BLOCK_HEIGHT * m_Indentation - CLE_BLOCK_HEIGHT / 2,
      pos.y() + height() - CLE_BLOCK_HEIGHT / 2,
      CLE_BLOCK_HEIGHT * (m_Indentation + 2),
      static_cast<int>(CLE_BLOCK_HEIGHT * 1.5));
}
