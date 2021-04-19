#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include "cle_action_block.h"

class CleScriptEditorBlock : public QWidget
{
  Q_OBJECT

public:
  CleScriptEditorBlock(QWidget *parent = nullptr);
  ~CleScriptEditorBlock();

public slots:
  void checkSnaps(CleActionBlock* position);
  void mousePressEvent(QMouseEvent *event) override;

private:
  std::vector<CleActionBlock*> blocks;
};

#endif
