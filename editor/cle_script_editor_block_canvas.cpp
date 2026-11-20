#include "cle_script_editor_block_canvas.h"
#include "cle_action_block_api.h"
#include "cle_action_block_bookend.h"
#include "cle_action_block_comparison.h"
#include "cle_action_block_ctrbinary.h"
#include "cle_action_block_ctrunary.h"
#include "cle_action_block_keyword.h"
#include "cle_action_block_nop.h"

extern "C"
{
  #include "../cl_script.h"
};

#include <QComboBox>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollArea>
#include <QScrollBar>

CleScriptEditorBlockCanvas::CleScriptEditorBlockCanvas(QWidget *parent)
  : QWidget(parent)
{
  /* If the C integration has an active script, build from that */
  if (buildFromScript() != CL_OK)
    buildNew();

  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  setAttribute(Qt::WA_OpaquePaintEvent);
  setFocusPolicy(Qt::StrongFocus);
  setAutoFillBackground(true);
}

void CleScriptEditorBlockCanvas::destroyBlocks(void)
{
  for (auto block : blocks)
    delete block;
  blocks.clear();
}

cl_error CleScriptEditorBlockCanvas::buildFromScript(void)
{
  if (!script.page_count ||
      !script.pages ||
      !script.pages[0].action_count ||
      !script.pages[0].actions)
    return CL_ERR_CLIENT_RUNTIME;

  destroyBlocks();

  auto start = new CleActionBlockBookend(false, this);
  connect(start, SIGNAL(onDrag(CleActionBlock*)),
          this, SLOT(checkSnaps(CleActionBlock*)));
  blocks.push_back(start);
  start->show();

  auto end = new CleActionBlockBookend(true, this);
  connect(end, SIGNAL(onDrag(CleActionBlock*)),
          this, SLOT(checkSnaps(CleActionBlock*)));
  blocks.push_back(end);
  end->show();

  auto prev = addBlock(&script.pages[0].actions[0]);
  unsigned i;

  prev->attachTo(start, 0);
  for (i = 1; i < script.pages[0].action_count; i++)
  {
    auto next = addBlock(&script.pages[0].actions[i]);
    next->attachTo(prev, script.pages[0].actions[i].indentation);
    prev = next;
  }
  end->attachTo(prev, 0);
  updateExtent();

  return CL_OK;
}

cl_error CleScriptEditorBlockCanvas::buildNew(void)
{
  destroyBlocks();

  auto start = new CleActionBlockBookend(false, this);
  connect(start, SIGNAL(onDrag(CleActionBlock*)),
          this, SLOT(checkSnaps(CleActionBlock*)));
  blocks.push_back(start);

  auto end = new CleActionBlockBookend(true, this);
  connect(end, SIGNAL(onDrag(CleActionBlock*)),
          this, SLOT(checkSnaps(CleActionBlock*)));
  blocks.push_back(end);

  cl_action_t *action = new cl_action_t;
  action->type = CL_ACTTYPE_ADDITION;
  auto middle = addBlock(action);

  middle->attachTo(start, 0);
  end->attachTo(middle, 0);
  updateExtent();

  return CL_OK;
}

CleActionBlock *CleScriptEditorBlockCanvas::addBlock(cl_action_t *action,
  QPoint pos)
{
  CleActionBlock *block = nullptr;
  QList<QComboBox*> boxes;
  int i;

  switch (action->type)
  {
  case CL_ACTTYPE_ADDITION:
  case CL_ACTTYPE_AND:
  case CL_ACTTYPE_DIVISION:
  case CL_ACTTYPE_MODULO:
  case CL_ACTTYPE_MULTIPLICATION:
  case CL_ACTTYPE_OR:
  case CL_ACTTYPE_SET:
  case CL_ACTTYPE_SHIFT_LEFT:
  case CL_ACTTYPE_SHIFT_RIGHT:
  case CL_ACTTYPE_SUBTRACTION:
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
  case CL_ACTTYPE_ELSE:
  case CL_ACTTYPE_LOGICAL_OR:
    block = new CleActionBlockKeyword(action, this);
    break;
  case CL_ACTTYPE_NO_PROCESS:
    block = new CleActionBlockNop(action, this);
  }
  if (block)
  {
    block->setType(action->type);
    block->populate();
    block->move(pos);
    connect(block, SIGNAL(onDrag(CleActionBlock*)),
            this, SLOT(checkSnaps(CleActionBlock*)));
    block->show();
    blocks.push_back(block);

    /* Combo boxes only take the wheel once clicked, so scrolling passes over them */
    boxes = block->findChildren<QComboBox*>();
    for (i = 0; i < boxes.size(); i++)
    {
      boxes[i]->setFocusPolicy(Qt::StrongFocus);
      boxes[i]->installEventFilter(this);
    }
    updateExtent();
  }

  return block;
}

CleActionBlock *CleScriptEditorBlockCanvas::addBlock(cl_action_t *action)
{
  return addBlock(action, QPoint(0, 0));
}

void CleScriptEditorBlockCanvas::checkSnaps(CleActionBlock* position)
{
  int indentation = -1;
  unsigned i;

  for (i = 0; i < blocks.size() && indentation < 0; i++)
    if (blocks[i] != position)
      indentation = blocks[i]->snapIndentation(position->pos());
  if (indentation >= 0)
    position->attachTo(blocks[i - 1], indentation);
  else
    position->detach();
  updateExtent();
}

void CleScriptEditorBlockCanvas::mousePressEvent(QMouseEvent *event)
{
  if (event->button() == Qt::RightButton)
  {
    QMenu subMenu(this);

    /* Add comparison actions */
    QAction *comparisonAction = subMenu.addAction("Compare values");
    comparisonAction->setData(CL_ACTTYPE_COMPARE);
    QAction *logicalOrAction = subMenu.addAction("OR (between two comparisons)");
    logicalOrAction->setData(CL_ACTTYPE_LOGICAL_OR);
    QAction *elseAction = subMenu.addAction("Else (after a comparison's actions)");
    elseAction->setData(CL_ACTTYPE_ELSE);

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
      cl_action_id type = static_cast<cl_action_id>(selectedSubAction->data().toInt());
      cl_action_t *action = new cl_action_t;

      memset(action, 0, sizeof(cl_action_t));
      action->type = type;
      addBlock(action, event->pos());
    }
  }
  /* Dragging empty space scrolls the view */
  else if ((event->button() == Qt::LeftButton ||
            event->button() == Qt::MiddleButton) && m_ScrollArea)
  {
    m_Panning = true;
    m_PanStart = event->globalPos();
    m_PanScroll = QPoint(m_ScrollArea->horizontalScrollBar()->value(),
                         m_ScrollArea->verticalScrollBar()->value());
    setCursor(Qt::ClosedHandCursor);
  }
}

void CleScriptEditorBlockCanvas::mouseMoveEvent(QMouseEvent *event)
{
  QPoint delta;

  if (!m_Panning || !m_ScrollArea)
    return;
  delta = event->globalPos() - m_PanStart;
  m_ScrollArea->horizontalScrollBar()->setValue(m_PanScroll.x() - delta.x());
  m_ScrollArea->verticalScrollBar()->setValue(m_PanScroll.y() - delta.y());
}

void CleScriptEditorBlockCanvas::mouseReleaseEvent(QMouseEvent *event)
{
  Q_UNUSED(event)
  if (m_Panning)
  {
    m_Panning = false;
    unsetCursor();
  }
}

bool CleScriptEditorBlockCanvas::eventFilter(QObject *object, QEvent *event)
{
  /* Ignored and filtered, the wheel event goes to the parents instead */
  if (event->type() == QEvent::Wheel && object->isWidgetType() &&
      !static_cast<QWidget*>(object)->hasFocus())
  {
    event->ignore();
    return true;
  }

  return QWidget::eventFilter(object, event);
}

void CleScriptEditorBlockCanvas::updateExtent(void)
{
  QRect area;
  int right = CLE_CANVAS_MIN_WIDTH;
  int bottom = CLE_CANVAS_MIN_HEIGHT;
  unsigned i;

  for (i = 0; i < blocks.size(); i++)
  {
    area = blocks[i]->geometry();
    right = qMax(right, area.right() + CLE_CANVAS_MARGIN);
    bottom = qMax(bottom, area.bottom() + CLE_CANVAS_MARGIN);
  }
  setMinimumSize(right, bottom);
}

void CleScriptEditorBlockCanvas::paintEvent(QPaintEvent *event)
{
  QPainter painter(this);
  painter.fillRect(event->rect(), parentWidget()->palette().color(QPalette::Window));
}

cle_result_t CleScriptEditorBlockCanvas::toString(void)
{
  CleActionBlock *next = nullptr;
  CleActionBlock *start = nullptr;
  QString string;
  unsigned actionCount = 0;
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
  next = start->next();
  if (!next)
    return { "No actions follow the start point", false };
  else if (next->isEnd())
    return { "No actions enclosed in start and end points", false };

  i = 1;
  do
  {
    /* Serialize this block */
    auto next_string = next->toString();
    if (!next_string.success)
    {
      next_string.text.prepend(QString("Error in action %1: ").arg(i));
      return next_string;
    }

    string += "\n" + next_string.text;
    actionCount++;

    next = next->next();
    i++;

    /* Sanity check: prevent infinite loops */
    if (actionCount > blocks.size())
      return { "Possible infinite recursion error", false };
  } while (next && !next->isEnd());

  /* Return final string only if the code block was closed with a bookend */
  if (!next || !next->isEnd())
    return { "Unable to find end point", false };

  /* Prepend number of pages and number of actions on the page */
  QString header = QString("%1\n%2").arg(1).arg(actionCount);  /** @todo Only 1 page for now */
  string = header + string;

  return { string, true };
}
