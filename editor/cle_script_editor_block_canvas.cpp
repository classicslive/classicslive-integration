#include "cle_script_editor_block_canvas.h"
#include "cle_action_block_api.h"
#include "cle_action_block_bookend.h"
#include "cle_action_block_comparison.h"
#include "cle_action_block_ctrbinary.h"
#include "cle_action_block_ctrunary.h"

extern "C"
{
  #include "../cl_script.h"
};

#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollArea>

CleScriptEditorBlockCanvas::CleScriptEditorBlockCanvas(QWidget *parent)
  : QWidget(parent)
{
  auto start = new CleActionBlockBookend(false, this);
  connect(start, SIGNAL(onDrag(CleActionBlock*)),
          this, SLOT(checkSnaps(CleActionBlock*)));
  blocks.push_back(start);

  auto end = new CleActionBlockBookend(true, this);
  connect(end, SIGNAL(onDrag(CleActionBlock*)),
          this, SLOT(checkSnaps(CleActionBlock*)));
  blocks.push_back(end);

  if (script.page_count &&
      script.pages &&
      script.pages[0].action_count &&
      script.pages[0].actions)
  {
    unsigned i;
    auto prev = addBlock(&script.pages[0].actions[0]);

    prev->attachTo(start, 0);
    for (i = 1; i < script.pages[0].action_count; i++)
    {
      auto next = addBlock(&script.pages[0].actions[i]);
      next->attachTo(prev, script.pages[0].actions[i].indentation);
      prev = next;
    }
    end->attachTo(prev, 0);
  }
  else
  {
    cl_action_t *action = new cl_action_t;
    action->type = CL_ACTTYPE_ADDITION;
    auto middle = addBlock(action);

    middle->attachTo(start, 0);
    end->attachTo(middle, 0);
  }

  setMinimumSize(640, 480);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  setAttribute(Qt::WA_OpaquePaintEvent);
  setFocusPolicy(Qt::StrongFocus);
  setAutoFillBackground(true);
}

CleActionBlock *CleScriptEditorBlockCanvas::addBlock(cl_action_t *action,
  QPoint pos)
{
  CleActionBlock *block = nullptr;

  switch (action->type)
  {
  case CL_ACTTYPE_ADDITION:
  case CL_ACTTYPE_SUBTRACTION:
  case CL_ACTTYPE_MULTIPLICATION:
  case CL_ACTTYPE_DIVISION:
  case CL_ACTTYPE_AND:
  case CL_ACTTYPE_OR:
  case CL_ACTTYPE_XOR:
    block = new CleActionBlockCtrBinary(action, this);
    break;
  case CL_ACTTYPE_COMPLEMENT:
    block = new CleActionBlockCtrUnary(action, this);
    break;
  case CL_ACTTYPE_POST_ACHIEVEMENT:
  case CL_ACTTYPE_POST_LEADERBOARD:
  case CL_ACTTYPE_POST_POLL:
  case CL_ACTTYPE_POST_PROGRESS:
    block = new CleActionBlockApi(action, this);
    break;
  case CL_ACTTYPE_COMPARE:
    block = new CleActionBlockComparison(action, this);
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

  return block;
}

CleActionBlock *CleScriptEditorBlockCanvas::addBlock(cl_action_t *action)
{
  return addBlock(action, QPoint(0, 0));
}

void CleScriptEditorBlockCanvas::checkSnaps(CleActionBlock* position)
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
        position->attachTo(blocks[i], indentation);
        return;
      }
    }
  }
  position->detach();
}

void CleScriptEditorBlockCanvas::mousePressEvent(QMouseEvent *event)
{
  if (event->button() == Qt::RightButton)
  {
    QMenu subMenu(this);

    /* Add comparison actions */
    QAction *comparisonAction = subMenu.addAction("Compare values");
    comparisonAction->setData(CL_ACTTYPE_COMPARE);

    QMenu *basicMathMenu = subMenu.addMenu("Basic Math");
    QMenu *bitwiseMenu = subMenu.addMenu("Bitwise Operations");

    /* Add basic math actions */
    QAction *additionAction = basicMathMenu->addAction("Addition");
    additionAction->setData(CL_ACTTYPE_ADDITION);
    QAction *subtractionAction = basicMathMenu->addAction("Subtraction");
    subtractionAction->setData(CL_ACTTYPE_SUBTRACTION);
    QAction *multiplicationAction = basicMathMenu->addAction("Multiplication");
    multiplicationAction->setData(CL_ACTTYPE_MULTIPLICATION);
    QAction *divisionAction = basicMathMenu->addAction("Division");
    divisionAction->setData(CL_ACTTYPE_DIVISION);

    /* Add bitwise actions */
    QAction *andAction = bitwiseMenu->addAction("AND");
    andAction->setData(CL_ACTTYPE_AND);
    QAction *orAction = bitwiseMenu->addAction("OR");
    orAction->setData(CL_ACTTYPE_OR);
    QAction *xorAction = bitwiseMenu->addAction("XOR");
    xorAction->setData(CL_ACTTYPE_XOR);
    QAction *notAction = bitwiseMenu->addAction("Complement");
    notAction->setData(CL_ACTTYPE_COMPLEMENT);

    /* Add API actions */
    QAction *achAction = subMenu.addAction("🏆 Unlock achievement");
    achAction->setData(CL_ACTTYPE_POST_ACHIEVEMENT);
    QAction *ldbAction = subMenu.addAction("📊 Post leaderboard entry");
    ldbAction->setData(CL_ACTTYPE_POST_LEADERBOARD);

    QAction *selectedSubAction = subMenu.exec(event->globalPos());

    if (selectedSubAction)
    {
      cl_action_t *action = new cl_action_t;
      int type = selectedSubAction->data().toInt();

      action->type = type;
      addBlock(action, event->pos());
    }
  }
}

void CleScriptEditorBlockCanvas::paintEvent(QPaintEvent *event)
{
  QPainter painter(this);
  painter.fillRect(event->rect(), parentWidget()->palette().color(QPalette::Background));
}

cle_result_t CleScriptEditorBlockCanvas::toString(void)
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
    return { "Unable to find start point", false };

  /* Iterate through all blocks after the starting bookend */
  i = 0;
  next = start->next();
  if (!next)
    return { "No actions follow the start point", false };
  else if (next->isEnd())
    return { "No actions enclosed in start and end points", false };
  else do
  {
    auto next_string = next->toString();

    if (next_string.success)
      string += "\n" + next->toString().text;
    else
      return next_string;
    next = next->next();
    i++;
    if (i > blocks.size())
      return { "Possible infinite recursion error", false };
  } while (next && !next->isEnd());

  /* Return final string only if the code block was closed with a bookend */
  if (next->isEnd())
    return { string, true };
  else
    return { "Unable to find end point", false };
}
