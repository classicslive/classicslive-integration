#include "cle_pointer_search_dialog.h"

extern "C"
{
  #include "../cl_memory.h"
}

ClePointerSearchDialog::ClePointerSearchDialog(QWidget *parent) : QDialog(parent)
{
  setWindowTitle(tr("Pointer Search Options"));

  pointerFollowsSpin = new QSpinBox(this);
  pointerFollowsSpin->setRange(1, CL_POINTER_MAX_PASSES);
  pointerFollowsSpin->setValue(4);

  offsetRangeSpin = new QSpinBox(this);
  offsetRangeSpin->setRange(0, 0xFFFFF);
  offsetRangeSpin->setValue(0x1000);
  offsetRangeSpin->setDisplayIntegerBase(16);

  maxMatchesSpin = new QSpinBox(this);
  maxMatchesSpin->setRange(1, 4000000);
  maxMatchesSpin->setValue(1000);

  maxMatchesPerPassSpin = new QSpinBox(this);
  maxMatchesPerPassSpin->setRange(0, 1000000);
  maxMatchesPerPassSpin->setValue(0);
  maxMatchesPerPassSpin->setSpecialValueText(tr("No limit"));

  QFormLayout *formLayout = new QFormLayout;
  formLayout->addRow(tr("Pointer Levels:"), pointerFollowsSpin);
  formLayout->addRow(tr("Offset Range (hex):"), offsetRangeSpin);
  formLayout->addRow(tr("Max Matches:"), maxMatchesSpin);
  formLayout->addRow(tr("Max Matches Per Pass:"), maxMatchesPerPassSpin);

  QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->addLayout(formLayout);
  mainLayout->addWidget(buttons);
}
