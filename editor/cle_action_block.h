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

  void attach(CleActionBlock *target, int indentation);

  void detach(void);

  int indentation(void) { return m_Indentation; }

  virtual bool isStart(void) { return false; }

  virtual bool isEnd(void) { return false; }

  virtual void setType(int type) { m_Type = type; }

  virtual QString toNopString(void)
  {
    return QString("%u %u 0 ")
      .arg(m_Indentation, 0, CL_RADIX)
      .arg(m_Type, 0, CL_RADIX);
  }

  virtual QString toString(void) { return "0 0 0"; }

  CleActionBlock* prev(void) { return m_Prev; }

  CleActionBlock* next(void) { return m_Next; }

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

  virtual void paintEvent(QPaintEvent *e) override;

signals:
  void onDrag(CleActionBlock* block);

protected:
  /**
   * The initial location when a drag is initiated. This is recorded to avoid
   * an infinite recursion error when looking up final position while dragging.
   */
  DragStart m_DragPos;

  cl_action_t *m_Action;
  int m_Indentation = 0;
  QHBoxLayout *m_Layout;
  CleActionBlock *m_Next = nullptr;
  CleActionBlock *m_Prev = nullptr;
  cle_block_snap m_SnapDirection;
  QRect m_SnapZone;
  int m_Type;
};

#endif
