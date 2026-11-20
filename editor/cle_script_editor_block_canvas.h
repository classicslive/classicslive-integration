#ifndef CLE_SCRIPT_EDITOR_BLOCK_CANVAS_H
#define CLE_SCRIPT_EDITOR_BLOCK_CANVAS_H

#include <QPushButton>
#include <QWidget>
#include <vector>

#include "cle_action_block.h"

class QScrollArea;

/* The smallest the canvas gets, and the empty space kept past the blocks */
#define CLE_CANVAS_MIN_WIDTH 640
#define CLE_CANVAS_MIN_HEIGHT 480
#define CLE_CANVAS_MARGIN (CLE_BLOCK_HEIGHT * 4)

class CleScriptEditorBlockCanvas : public QWidget
{
  Q_OBJECT

public:
  CleScriptEditorBlockCanvas(QWidget *parent = nullptr);

  cl_error buildFromScript(void);
  cl_error buildNew(void);

  cle_result_t toString(void);

  /**
   * Sets the scroll area showing this canvas, which dragging on empty space
   * scrolls.
   * @param area The scroll area, or nullptr to disable drag-scrolling.
   */
  void setScrollArea(QScrollArea *area) { m_ScrollArea = area; }

  /**
   * Passes wheel events over unfocused combo boxes on to the scroll area, so
   * scrolling past a block doesn't change its values.
   */
  bool eventFilter(QObject *object, QEvent *event) override;

public slots:
  void checkSnaps(CleActionBlock* position);

  /* QWidget overrides */
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;

protected:
  void paintEvent(QPaintEvent *event) override;

private:
  std::vector<CleActionBlock*> blocks;

  QScrollArea *m_ScrollArea = nullptr;

  /* Whether empty space is being dragged to scroll, and where it started */
  bool m_Panning = false;
  QPoint m_PanStart;
  QPoint m_PanScroll;

  CleActionBlock *addBlock(cl_action_t *action);
  CleActionBlock *addBlock(cl_action_t *action, QPoint pos);
  void destroyBlocks(void);

  /** Grows or shrinks the canvas to fit its blocks, plus a margin. */
  void updateExtent(void);
};

#endif
