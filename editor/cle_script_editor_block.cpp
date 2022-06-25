#include "cle_script_editor_block.h"
#include "cle_action_block_bookend.h"
#include "cle_action_block_ctrbinary.h"

#include <QMouseEvent>

CleScriptEditorBlock::CleScriptEditorBlock(QWidget *parent)
  : QWidget(parent)
{
  auto start = new CleActionBlockBookend(this, false);
  connect(start, SIGNAL(onDrag(CleActionBlock*)),
          this,  SLOT(checkSnaps(CleActionBlock*)));
  blocks.push_back(start);

  auto end = new CleActionBlockBookend(this, true);
  connect(end,  SIGNAL(onDrag(CleActionBlock*)),
          this, SLOT(checkSnaps(CleActionBlock*)));
  blocks.push_back(end);

  addBlock(CL_ACTTYPE_ADDITION);
}

CleScriptEditorBlock::~CleScriptEditorBlock()
{
}

void CleScriptEditorBlock::addBlock(uint8_t type)
{
  CleActionBlock* block;

  switch (type)
  {
  case CL_ACTTYPE_ADDITION:
    block = new CleActionBlockCtrBinary(type, this);
    break;
  }

  connect(block, SIGNAL(onDrag(CleActionBlock*)),
          this,  SLOT(checkSnaps(CleActionBlock*)));
  blocks.push_back(block);
}

void CleScriptEditorBlock::checkSnaps(CleActionBlock* position)
{
  for (size_t i = 0; i < blocks.size(); i++)
  {
    if (blocks[i] == position)
      continue;
    else
    {
      auto indentation = blocks[i]->getSnapArea(position->pos());

      if (indentation >= 0)
      {
        position->attach(blocks[i], indentation);
        return;
      }
    }
  }
  position->detach();
}

void CleScriptEditorBlock::mousePressEvent(QMouseEvent *event)
{
  if (event->button() == Qt::RightButton)
  {
    auto a = new CleActionBlockCtrBinary(CL_ACTTYPE_ADDITION, this);
    connect(a, SIGNAL(onDrag(CleActionBlock*)), this, SLOT(checkSnaps(CleActionBlock*)));
    a->show();
    blocks.push_back(a);
  }
}

QString CleScriptEditorBlock::toString()
{
  const QString   error  = "0";
  CleActionBlock* next   = nullptr;
  CleActionBlock* start  = nullptr;
  QString         string = QString::number(1, 16);

  /* Find our starting point */
  for (int i = 0; i < blocks.size(); i++)
  {
    if (blocks[i]->isStart())
    {
      start = blocks[i];
      break;
    }
  }
  if (!start)
    return error;

  /* Iterate through all blocks after the starting bookend */
  next = start->getNext();
  if (!next)
    return error;
  else do
  {
    string += " " + next->toString();
    next = next->getNext();
  } while (next && !next->isEnd());

  /* Return final string only if the code block was closed with a bookend */
  return next->isEnd() ? string : error;
}
