#include "cle_script_editor_block.h"

extern "C"
{
  #include "../cl_network.h"
  #include "../cl_script.h"
}

#include <QMessageBox>
#include <QUrlQuery>

static void cle_script_upload_cb(cl_network_response_t response, void *ud)
{
  auto script = (CleScriptEditorBlock*)ud;

  if (!response.error_code)
  {
    auto string = script->script().toStdString();
    const char *pos = string.c_str();

    cl_script_free();
    cl_script_init(&pos);
    script->rebuild();
    QMessageBox::information(nullptr, "Upload result",
                             "CL Script uploaded successfully!");
  }
  if (script)
    script->setEnabled(true);
}

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

    m_Script = result.text.replace('\n', ' ');
    query.addQueryItem("script_id", "core");
    query.addQueryItem("script", m_Script);
    post = query.query(QUrl::FullyEncoded).toUtf8();
    cl_network_post_api(CL_END_SCRIPT_EDIT, post.data(), cle_script_upload_cb,
                        this);
    setEnabled(false);
  }
  else
    QMessageBox::warning(this, "CL Script error", result.text);
}
