#include "cle_action_block_api.h"

extern "C"
{
  #include "../cl_main.h"
  #include "../cl_memory.h"
  #include "../cl_network.h"
  #include "../cl_script.h"
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
  unsigned i;

  /* Operand label */
  m_Label = new QLabel(this);
  m_Layout->addWidget(m_Label);

  /* ID source; onChangeSource picks the matching value widget */
  m_SourceType = new QComboBox(this);
  m_SourceType->addItem(tr("constant"), CL_SRCTYPE_IMMEDIATE_INT);
  m_SourceType->addItem(tr("counter"), CL_SRCTYPE_COUNTER);
  m_SourceType->addItem(tr("current"), CL_SRCTYPE_CURRENT_RAM);
  m_SourceType->addItem(tr("previous"), CL_SRCTYPE_PREVIOUS_RAM);
  m_SourceType->addItem(tr("last unique"), CL_SRCTYPE_LAST_UNIQUE_RAM);
  m_SourceType->setToolTip(tr("Where the ID comes from. A counter or memory "
    "note lets one action unlock a different achievement each time."));
  m_Layout->addWidget(m_SourceType);

  /* ComboBox with editable field (achievements) */
  m_ComboBox = new QComboBox(this);
  m_ComboBox->setEditable(true);
  m_ComboBox->setInsertPolicy(QComboBox::NoInsert);

  // Populate with available achievements
  for (i = 0; i < session.achievement_count; i++)
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

  /* Counter index and memory note pickers for the other sources */
  m_CounterEdit = new QLineEdit(this);
  m_CounterEdit->setPlaceholderText(tr("Counter index"));
  m_NoteComboBox = new QComboBox(this);
  populateNoteComboBox(m_NoteComboBox);

  m_ValueStack = new QStackedWidget(this);
  m_ValueStack->insertWidget(CLE_API_VALUE_CONSTANT, m_ComboBox);
  m_ValueStack->insertWidget(CLE_API_VALUE_COUNTER, m_CounterEdit);
  m_ValueStack->insertWidget(CLE_API_VALUE_NOTE, m_NoteComboBox);
  m_Layout->addWidget(m_ValueStack);
  connect(m_SourceType, SIGNAL(currentIndexChanged(int)),
          this, SLOT(onChangeSource(int)));

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
  QUrl url(QString(CL_URL_SITE "/%1").arg(QString::fromUtf8(ach->icon_url)));

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

void CleActionBlockApi::onChangeSource(int index)
{
  unsigned source = m_SourceType->itemData(index).toUInt();
  int page;

  if (source == CL_SRCTYPE_IMMEDIATE_INT)
    page = CLE_API_VALUE_CONSTANT;
  else if (source == CL_SRCTYPE_COUNTER)
    page = CLE_API_VALUE_COUNTER;
  else
    page = CLE_API_VALUE_NOTE;
  m_ValueStack->setCurrentIndex(page);

  /* The icon only means something for a fixed achievement */
  m_ImageLabel->setVisible(page == CLE_API_VALUE_CONSTANT);
}

void CleActionBlockApi::populate(void)
{
  unsigned source, current_id, i;
  int index;

  if (!m_Action || m_Action->argument_count < 2)
    return;

  source = m_Action->arguments[0].uintval;
  current_id = m_Action->arguments[1].uintval;
  index = m_SourceType->findData(source);
  m_SourceType->setCurrentIndex(index >= 0 ? index : 0);
  onChangeSource(m_SourceType->currentIndex());

  if (m_ValueStack->currentIndex() == CLE_API_VALUE_COUNTER)
  {
    m_CounterEdit->setText(QString::number(current_id));
    return;
  }
  else if (m_ValueStack->currentIndex() == CLE_API_VALUE_NOTE)
  {
    index = m_NoteComboBox->findData(current_id);
    if (index >= 0)
      m_NoteComboBox->setCurrentIndex(index);
    return;
  }

  m_ComboBox->setEditText(QString::number(current_id));
  if (m_Action->type == CL_ACTTYPE_POST_ACHIEVEMENT ||
      m_Action->type == CL_ACTTYPE_POST_PROGRESS)
  {
    for (i = 0; i < session.achievement_count; i++)
    {
      if (session.achievements[i].id == current_id)
      {
        m_ComboBox->setCurrentIndex(i);
        return;
      }
    }
  }
  else if (m_Action->type == CL_ACTTYPE_POST_LEADERBOARD)
  {
    for (i = 0; i < session.leaderboard_count; i++)
    {
      if (session.leaderboards[i].id == current_id)
      {
        m_ComboBox->setCurrentIndex(i);
        return;
      }
    }
  }
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

unsigned CleActionBlockApi::constantId(bool *ok)
{
  QVariant data = m_ComboBox->currentData();
  unsigned id = 0;

  *ok = false;
  if (data.isValid())
    id = data.toUInt(ok);

  /* Fall back to a typed number */
  if (!*ok)
    id = static_cast<unsigned>(stringToValue(m_ComboBox->currentText(), ok));

  return *ok ? id : 0;
}

cle_result_t CleActionBlockApi::toString(void)
{
  unsigned source = m_SourceType->currentData().toUInt();
  int64_t value = 0;
  bool ok = false;

  switch (m_ValueStack->currentIndex())
  {
  case CLE_API_VALUE_CONSTANT:
    value = constantId(&ok);
    break;
  case CLE_API_VALUE_COUNTER:
    value = stringToValue(m_CounterEdit->text(), &ok);
    ok = ok && value >= 0 && value < CL_COUNTERS_SIZE;
    break;
  case CLE_API_VALUE_NOTE:
    value = m_NoteComboBox->currentData().toUInt(&ok);
    break;
  }
  if (!ok)
    return { "Empty or invalid values", false };

  QString string = QString("%1 %2 2 %3 %4")
    .arg(indentation(), 0, CL_RADIX)
    .arg(type(), 0, CL_RADIX)
    .arg(source)
    .arg(value, 0, CL_RADIX);

  return { string, true };
}

