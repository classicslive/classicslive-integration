#ifndef CLE_POINTER_SEARCH_DIALOG_H
#define CLE_POINTER_SEARCH_DIALOG_H

#include <QComboBox>
#include <QDialog>
#include <QSpinBox>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QLabel>

extern "C"
{
  #include "../cl_types.h"
}

class ClePointerSearchDialog : public QDialog
{
  Q_OBJECT

public:
  ClePointerSearchDialog(QWidget *parent = nullptr);

  int pointerFollows(void) const { return pointerFollowsSpin->value(); }
  int offsetRange(void) const { return offsetRangeSpin->value(); }
  int maxMatches(void) const { return maxMatchesSpin->value(); }
  int maxMatchesPerPass(void) const { return maxMatchesPerPassSpin->value(); }

  /* The region pointer chains must start in, or nullptr for any region */
  const cl_memory_region_t *baseRegion(void) const;

private:
  QSpinBox *pointerFollowsSpin = nullptr;
  QSpinBox *offsetRangeSpin = nullptr;
  QSpinBox *maxMatchesSpin = nullptr;
  QSpinBox *maxMatchesPerPassSpin = nullptr;
  QComboBox *baseRegionCombo = nullptr;
};

#endif
