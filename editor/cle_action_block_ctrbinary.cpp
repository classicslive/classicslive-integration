#ifndef CLE_ACTION_BLOCK_CTRBINARY
#define CLE_ACTION_BLOCK_CTRBINARY

#include "cle_action_block_ctrbinary.h"

QString CleActionBlockCtrBinary::toString()
{
  return QString("3 " + QString::number(m_Type, 16));
}

#endif