#ifndef CLE_MAIN_H
#define CLE_MAIN_H

#include <QtWidgets/QApplication>

#include "cle_memory_inspector.h"
#include "cle_script_editor.h"

QApplication       *m_Application;
CleMemoryInspector *m_MemoryInspector;
CleScriptEditor    *m_ScriptEditor;

#endif
