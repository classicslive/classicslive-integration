#include "cle_script_editor_block.h"
#include "cle_action_block_bookend.h"
#include "cle_action_block_ctrbinary.h"

#include <QMenu>
#include <QMouseEvent>

CleScriptEditorBlock::CleScriptEditorBlock(QWidget *parent)
  : QWidget(parent)
{
  resize(640, 480);
  auto start = new CleActionBlockBookend(this, false);
  connect(start, SIGNAL(onDrag(CleActionBlock*)),
          this, SLOT(checkSnaps(CleActionBlock*)));
  blocks.push_back(start);

  auto end = new CleActionBlockBookend(this, true);
  connect(end, SIGNAL(onDrag(CleActionBlock*)),
          this, SLOT(checkSnaps(CleActionBlock*)));
  blocks.push_back(end);

  addBlock(CL_ACTTYPE_ADDITION);
}

CleScriptEditorBlock::~CleScriptEditorBlock()
{
}

void CleScriptEditorBlock::addBlock(int type, QPoint pos)
{
  CleActionBlock* block = nullptr;

  switch (type)
  {
  case CL_ACTTYPE_ADDITION:
  case CL_ACTTYPE_SUBTRACTION:
  case CL_ACTTYPE_MULTIPLICATION:
  case CL_ACTTYPE_DIVISION:
  case CL_ACTTYPE_AND:
  case CL_ACTTYPE_OR:
  case CL_ACTTYPE_XOR:
  case CL_ACTTYPE_COMPLEMENT:
    block = new CleActionBlockCtrBinary(type, this);
    break;
  }
  if (block)
  {
    block->move(pos);
    connect(block, SIGNAL(onDrag(CleActionBlock*)),
            this, SLOT(checkSnaps(CleActionBlock*)));
    block->show();
    blocks.push_back(block);
  }
}

void CleScriptEditorBlock::addBlock(int type)
{
  addBlock(type, QPoint(0, 0));
}

void CleScriptEditorBlock::checkSnaps(CleActionBlock* position)
{
  unsigned i;

  for (i = 0; i < blocks.size(); i++)
  {
    if (blocks[i] == position)
      continue;
    else
    {
      auto indentation = blocks[i]->snapIndentation(position->pos());

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
    QMenu subMenu(this);
    QMenu *basicMathMenu = subMenu.addMenu("Basic Math");
    QMenu *bitwiseMenu = subMenu.addMenu("Bitwise Operations");

    // Add Basic Math Options
    QAction *additionAction = basicMathMenu->addAction("Addition");
    additionAction->setData(CL_ACTTYPE_ADDITION);
    QAction *subtractionAction = basicMathMenu->addAction("Subtraction");
    subtractionAction->setData(CL_ACTTYPE_SUBTRACTION);
    QAction *multiplicationAction = basicMathMenu->addAction("Multiplication");
    multiplicationAction->setData(CL_ACTTYPE_MULTIPLICATION);
    QAction *divisionAction = basicMathMenu->addAction("Division");
    divisionAction->setData(CL_ACTTYPE_DIVISION);

    // Add Bitwise Options
    QAction *andAction = bitwiseMenu->addAction("AND");
    andAction->setData(CL_ACTTYPE_AND);
    QAction *orAction = bitwiseMenu->addAction("OR");
    orAction->setData(CL_ACTTYPE_OR);
    QAction *xorAction = bitwiseMenu->addAction("XOR");
    xorAction->setData(CL_ACTTYPE_XOR);
    QAction *notAction = bitwiseMenu->addAction("Complement");
    notAction->setData(CL_ACTTYPE_COMPLEMENT);

    // Add special options
    QAction *achAction = subMenu.addAction("🏆 Unlock achievement");
    achAction->setData(CL_ACTTYPE_POST_ACHIEVEMENT);
    QAction *ldbAction = subMenu.addAction("📊 Post leaderboard entry");
    ldbAction->setData(CL_ACTTYPE_POST_LEADERBOARD);

    QAction *selectedSubAction = subMenu.exec(event->globalPos());

    if (selectedSubAction)
    {
      int actionType = selectedSubAction->data().toInt();
      addBlock(actionType, event->pos());
    }
  }
}

QString CleScriptEditorBlock::toString()
{
  const QString error = "0";
  CleActionBlock *next = nullptr;
  CleActionBlock *start = nullptr;
  QString string = QString::number(1, 16);
  unsigned i;

  /* Find our starting point */
  for (i = 0; i < blocks.size(); i++)
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
  next = start->next();
  if (!next)
    return error;
  else do
  {
    string += " " + next->toString();
    next = next->next();
  } while (next && !next->isEnd());

  /* Return final string only if the code block was closed with a bookend */
  return next->isEnd() ? string : error;
}
