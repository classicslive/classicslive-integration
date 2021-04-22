#ifndef CLE_SCRIPT_EDITOR_BLOCK_CPP
#define CLE_SCRIPT_EDITOR_BLOCK_CPP

#include "cle_script_editor_block.h"

#include <QMouseEvent>

CleScriptEditorBlock::CleScriptEditorBlock(QWidget *parent)
  : QWidget(parent)
{
  auto a = new CleActionBlock(this);
  connect(a, SIGNAL(onDrag(CleActionBlock*)), this, SLOT(checkSnaps(CleActionBlock*)));
  blocks.push_back(a);
}

CleScriptEditorBlock::~CleScriptEditorBlock()
{
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
    auto a = new CleActionBlock(this);
    connect(a, SIGNAL(onDrag(CleActionBlock*)), this, SLOT(checkSnaps(CleActionBlock*)));
    a->show();
    blocks.push_back(a);
  }
}

#endif
