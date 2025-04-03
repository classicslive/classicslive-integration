#include "cle_script_editor_block.h"

#include <QMessageBox>

CleScriptEditorBlock::CleScriptEditorBlock(QWidget *parent)
  : QWidget(parent)
{
  m_Canvas = new CleScriptEditorBlockCanvas(this);

  m_SaveButton = new QPushButton("Save", this);
  connect(m_SaveButton, SIGNAL(clicked()), this, SLOT(onSaveButtonClicked()));

  auto layout = new QVBoxLayout(this);
  layout->addWidget(m_Canvas);
  layout->addWidget(m_SaveButton);

  setLayout(layout);
}

void CleScriptEditorBlock::onSaveButtonClicked(void)
{
  cle_result_t result = m_Canvas->toString();

  if (result.success)
    QMessageBox::information(this, "CLScript result", result.text);
  else
    QMessageBox::warning(this, "CLScript error", result.text);
}
