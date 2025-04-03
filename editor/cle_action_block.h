#ifndef CLE_ACTION_BLOCK_H
#define CLE_ACTION_BLOCK_H

#include <QGroupBox>
#include <QHBoxLayout>
#include <QWidget>

#include "cle_common.h"

extern "C"
{
   #include "../cl_action.h"
}

/**
 * Used to determine which side of the element should be highlighted when
 * snapping to another.
 */
typedef enum
{
  CLE_SNAP_NONE = 0,
  CLE_SNAP_UP,
  CLE_SNAP_DOWN,
  CLE_SNAP_LEFT,
  CLE_SNAP_RIGHT,

  CLE_SNAP_SIZE
} cle_block_snap;

#define CLE_BLOCK_WIDTH 512
#define CLE_BLOCK_HEIGHT 32

typedef struct
{
  QPoint global;
  QPoint parent;
  QPoint self;
} DragStart;

class CleActionBlock : public QWidget
{
  Q_OBJECT

public:
  CleActionBlock(QWidget* parent);

  ~CleActionBlock() override;

  void attachTo(CleActionBlock *target, int indentation);

  void detach(void);

  int indentation(void) { return m_Indentation; }

  virtual bool isStart(void) { return false; }

  virtual bool isEnd(void) { return false; }

  virtual void setType(int type) { m_Type = type; }

  virtual cle_result_t toString(void) { return { "Uninitialized action", false }; }

  CleActionBlock* prev(void) { return m_Prev; }

  CleActionBlock* next(void) { return m_Next; }

  bool selected(void) { return m_Selected; }

  void select(void) { m_Selected = true; }

  void selectWithChildren(void) { m_Selected = true; }

  void deselect(void) { m_Selected = false; }

  void deselectWithChildren(void) { m_Selected = false; }

  /**
   * Returns which indentation level the specified point will snap into, or -1
   * if it is not in range.
   */
  virtual int snapIndentation(QPoint pos);

  QRect snapZone(void) { return m_SnapZone; }

  void setNext(CleActionBlock *next) { m_Next = next; }

  void setPrev(CleActionBlock *prev) { m_Prev = prev; }

  void setPosition(QPoint pos);

public slots:
  void onSnap(const QPoint *position, cle_block_snap direction);

  virtual void mouseMoveEvent(QMouseEvent *event) override;

  virtual void mousePressEvent(QMouseEvent *event) override;

  virtual void mouseReleaseEvent(QMouseEvent *event) override;

  virtual void paintEvent(QPaintEvent *e) override;

signals:
  void onDrag(CleActionBlock* block);

protected:
  /**
   * The initial location when a drag is initiated. This is recorded to avoid
   * an infinite recursion error when looking up final position while dragging.
   */
  DragStart m_DragPos;

  cl_action_t *m_Action = nullptr;
  int m_Indentation = 0;
  QHBoxLayout *m_Layout = nullptr;
  bool m_Selected = false;
  cle_block_snap m_SnapDirection = CLE_SNAP_NONE;
  QRect m_SnapZone;
  int m_Type = 0;

  CleActionBlock *m_Next = nullptr;
  CleActionBlock *m_Prev = nullptr;
};

#endif
