#include "cle_script_editor_block.h"

extern "C"
{
  #include "../cl_network.h"
}

#include <QMessageBox>
#include <QUrlQuery>

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
  {
    QUrlQuery query;
    QByteArray post;

    query.addQueryItem("script_id", "core");
    query.addQueryItem("script", result.text);
    post = query.query(QUrl::FullyEncoded).toUtf8();
    cl_network_post_api(CL_END_SCRIPT_EDIT, post.data(), nullptr, nullptr);
    QMessageBox::information(this, "CLScript result", result.text);
  }
  else
    QMessageBox::warning(this, "CLScript error", result.text);
}
