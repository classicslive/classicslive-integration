#ifndef CLE_POINTER_SEARCH_DIALOG_H
#define CLE_POINTER_SEARCH_DIALOG_H

#include <QDialog>
#include <QSpinBox>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QLabel>

class ClePointerSearchDialog : public QDialog
{
  Q_OBJECT

public:
  ClePointerSearchDialog(QWidget *parent = nullptr);

  int pointerFollows(void) const { return pointerFollowsSpin->value(); }
  int offsetRange(void) const { return offsetRangeSpin->value(); }
  int maxMatches(void) const { return maxMatchesSpin->value(); }

private:
  QSpinBox *pointerFollowsSpin = nullptr;
  QSpinBox *offsetRangeSpin = nullptr;
  QSpinBox *maxMatchesSpin = nullptr;
};

#endif
