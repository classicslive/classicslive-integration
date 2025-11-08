#include "cle_action_block_api.h"

extern "C"
{
  #include "../cl_main.h"
}

#include <QCompleter>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QPixmap>
#include <QImage>
#include <QColor>

CleActionBlockApi::CleActionBlockApi(cl_action_t *action, QWidget* parent)
  : CleActionBlock(action, parent)
{
  /* Operand label */
  m_Label = new QLabel(this);
  m_Layout->addWidget(m_Label);

  /* ComboBox with editable field (achievements) */
  m_ComboBox = new QComboBox(this);
  m_ComboBox->setEditable(true);
  m_ComboBox->setInsertPolicy(QComboBox::NoInsert);

  // Populate with available achievements
  for (unsigned i = 0; i < session.achievement_count; i++)
  {
    const cl_achievement_t* ach = &session.achievements[i];
    QString title = QString::fromUtf8(ach->title);
    QString desc = QString::fromUtf8(ach->description).replace("\\n", "\n");

    m_ComboBox->addItem(title, QVariant::fromValue(ach->id));
    m_ComboBox->setItemData(i, desc, Qt::ToolTipRole);
  }

  connect(m_ComboBox, SIGNAL(activated(int)),
          this, SLOT(onChanged(int)));
  connect(m_ComboBox->lineEdit(), SIGNAL(textEdited(QString)),
          this, SLOT(onEdited(QString)));

  m_Layout->addWidget(m_ComboBox);

  /* Image label */
  m_Image = QImage(256, 256, QImage::Format_ARGB32);
  m_Image.fill(Qt::red);
  m_Image = m_Image.scaled(CLE_BLOCK_HEIGHT, CLE_BLOCK_HEIGHT);
  m_ImageLabel = new QLabel(this);
  m_ImageLabel->setPixmap(QPixmap::fromImage(m_Image));
  m_Layout->addWidget(m_ImageLabel);

  if (!m_Action->arguments)
  {
    m_Action->argument_count = 2;
    m_Action->arguments = (cl_arg_t*)calloc(m_Action->argument_count, sizeof(cl_arg_t));
    m_Owned = true;
  }

  setLayout(m_Layout);
}

/* Handle text edits or typed numbers */
void CleActionBlockApi::onEdited(const QString& text)
{
  bool ok = false;
  unsigned index = stringToValue(text, &ok);
  if (ok)
    setAchievementIconByIndex(index);
}

/* Handle dropdown selection */
void CleActionBlockApi::onChanged(int index)
{
  if (index < 0 || (unsigned)index >= session.achievement_count)
    return;

  const cl_achievement_t* ach = &session.achievements[index];
  setAchievementIconByIndex(index);
  m_ComboBox->setToolTip(QString::fromUtf8(ach->description).replace("\\n", "\n"));
}

/* Helper to color or load image */
void CleActionBlockApi::setAchievementIconByIndex(unsigned index)
{
  if (index >= session.achievement_count || session.achievements[index].icon_url[0] == '\0')
  {
    // Fallback: colorize based on index
    QColor color((index * 50) % 255, (index * 120) % 255, (index * 200) % 255);
    m_Image.fill(color);
    m_Image = m_Image.scaled(CLE_BLOCK_HEIGHT, CLE_BLOCK_HEIGHT);
    m_ImageLabel->setPixmap(QPixmap::fromImage(m_Image));
    return;
  }

  // Load from URL
  const cl_achievement_t *ach = &session.achievements[index];
  QUrl url("http://classicslive.doggylongface.com/images/game_icon/9026eb2ec1298be33a7c18a2b13620f7.jpeg");

  // Make sure network manager exists
  static QNetworkAccessManager *manager = new QNetworkAccessManager(this);

  QNetworkReply *reply = manager->get(QNetworkRequest(url));

  // Use a lambda to handle the reply asynchronously
  connect(reply, &QNetworkReply::finished, this, [this, reply]() {
    if (reply->error() == QNetworkReply::NoError)
    {
      QByteArray data = reply->readAll();
      QPixmap pix;
      if (pix.loadFromData(data))
      {
        m_ImageLabel->setPixmap(pix.scaled(CLE_BLOCK_HEIGHT, CLE_BLOCK_HEIGHT,
                                           Qt::KeepAspectRatio, Qt::SmoothTransformation));
      }
      else
      {
        // Failed to decode image, fallback to gray
        m_Image.fill(Qt::gray);
        m_ImageLabel->setPixmap(QPixmap::fromImage(m_Image));
      }
    }
    else
    {
      // Network error, fallback to gray
      m_Image.fill(Qt::gray);
      m_ImageLabel->setPixmap(QPixmap::fromImage(m_Image));
    }
    reply->deleteLater();
  });
}

void CleActionBlockApi::populate(void)
{
  unsigned current_id = m_Action->arguments[1].uintval;
  for (unsigned i = 0; i < session.achievement_count; i++)
  {
    if (session.achievements[i].id == current_id)
    {
      m_ComboBox->setCurrentIndex(i);
      return;
    }
  }
  m_ComboBox->setEditText(QString::number(current_id));
}

void CleActionBlockApi::setType(cl_action_id type)
{
  switch (type)
  {
  case CL_ACTTYPE_POST_ACHIEVEMENT:
    m_Label->setText("Unlock...");
    break;
  case CL_ACTTYPE_POST_LEADERBOARD:
    m_Label->setText("Post entry to...");
    break;
  case CL_ACTTYPE_POST_PROGRESS:
    m_Label->setText("Post progress for...");
    break;
  case CL_ACTTYPE_POST_POLL:
    m_Label->setText("Submit poll...");
    break;
  default:
    m_Label->setText("Invalid API action " + QString::number(type));
  }
  CleActionBlock::setType(type);
}

cle_result_t CleActionBlockApi::toString(void)
{
  unsigned id = 0;
  bool ok = false;

  // Try to get the data from the combo box's current index
  QVariant data = m_ComboBox->currentData();
  if (data.isValid())
    id = data.toUInt(&ok);

  // Fallback: parse the text if data is invalid or not ok
  if (!ok)
    id = stringToValue(m_ComboBox->currentText(), &ok);

  if (!ok)
    return { "Empty or invalid values", false };

  QString string = QString("%1 %2 2 %3 %4")
    .arg(indentation(), 0, CL_RADIX)
    .arg(type(), 0, CL_RADIX)
    .arg(CL_SRCTYPE_IMMEDIATE_INT)
    .arg(id, 0, CL_RADIX);

  return { string, true };
}

