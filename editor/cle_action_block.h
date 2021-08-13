#ifndef CLE_ACTION_BLOCK_H
#define CLE_ACTION_BLOCK_H

#include <QGroupBox>
#include <QHBoxLayout>
#include <QWidget>

#include "cle_common.h"

/*
  Used to determine which side of the element should be highlighted when
  snapping to another.
*/
#define CLE_ACT_SNAP_NONE  0
#define CLE_ACT_SNAP_UP    1
#define CLE_ACT_SNAP_DOWN  2
#define CLE_ACT_SNAP_LEFT  3
#define CLE_ACT_SNAP_RIGHT 4

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

  void attach(CleActionBlock *target, char indentation);
  void detach();
  char getIndentation() { return m_Indentation; }
  virtual bool isStart() { return false; }
  virtual bool isEnd() { return false; }
  virtual void setType(uint8_t type) { m_Type = type; }

  virtual QString toString() { return "0 30 0"; }

  CleActionBlock* getPrev() { return m_Prev; }
  CleActionBlock* getNext() { return m_Next; }

  /*
    Returns which indentation level the specified point will snap into, or -1
    if it is not in range.
  */
  char getSnapArea(QPoint pos);

  QRect getSnapZone() { return m_SnapZone; }

  void setNext(CleActionBlock *next) { m_Next = next; }
  void setPrev(CleActionBlock *prev) { m_Prev = prev; }
  void setPosition(QPoint pos);

public slots:
  void onSnap(const QPoint *position, unsigned char direction);

  virtual void mouseMoveEvent(QMouseEvent *event) override;
  virtual void mousePressEvent(QMouseEvent *event) override;
  virtual void paintEvent(QPaintEvent *e) override;

signals:
  void onDrag(CleActionBlock* block);

protected:
  /*
    The initial location when a drag is initiated. This is recorded to avoid
    an infinite recursion error when looking up final position while dragging.
  */
  DragStart m_DragPos;

  cl_action_t    *m_Action;
  char            m_Indentation = 0;
  QHBoxLayout    *m_Layout;
  CleActionBlock *m_Next = nullptr;
  CleActionBlock *m_Prev = nullptr;
  unsigned char   m_SnapDirection;
  QRect           m_SnapZone;
  unsigned char   m_Type;
};

#endif
