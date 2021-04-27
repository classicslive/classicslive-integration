#ifndef CLE_ACTION_BLOCK_BOOKEND_CPP
#define CLE_ACTION_BLOCK_BOOKEND_CPP

#include "cle_action_block_bookend.h"

#include <QPainter>

CleActionBlockBookend::CleActionBlockBookend(QWidget* parent) : CleActionBlock(parent)
{
  setParent(parent);
}

void CleActionBlockBookend::paintEvent(QPaintEvent *e)
{
  QPainter painter(this);

  /* Draw a light border around all elements */
  painter.setPen(Qt::NoPen);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setBrush(QColor(15, 99, 179));
  painter.drawRoundedRect(QRectF(0, 0, CLE_BLOCK_WIDTH / 2, CLE_BLOCK_HEIGHT), 2.0, 2.0);
}

#endif
