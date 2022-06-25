#include "cle_action_block_bookend.h"

#include <QPainter>

CleActionBlockBookend::CleActionBlockBookend(QWidget* parent, bool is_end) : CleActionBlock(parent)
{
  m_IsEnd = is_end;
  setGeometry(0, 0, CLE_BLOCK_WIDTH / 2, CLE_BLOCK_HEIGHT * 1.5);
}

void CleActionBlockBookend::paintEvent(QPaintEvent *e)
{
  QPainter painter(this);

  painter.setPen(Qt::NoPen);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setBrush(QColor(15, 99, 179));

  if (m_IsEnd)
    painter.drawRoundedRect(QRectF(0, 0, CLE_BLOCK_WIDTH / 2, CLE_BLOCK_HEIGHT), 2.0, 2.0);
  else
    painter.drawRoundedRect(QRectF(0, CLE_BLOCK_HEIGHT * 0.5, CLE_BLOCK_WIDTH / 2, CLE_BLOCK_HEIGHT), 2.0, 2.0);
  
  painter.drawEllipse(QRectF(0, 0, CLE_BLOCK_WIDTH / 4, CLE_BLOCK_HEIGHT * 1.5));

  painter.setPen(Qt::white);
  painter.drawText(16, m_IsEnd ? 20 : 36, m_IsEnd ? "End CLScript" : "Begin CLScript");
}
