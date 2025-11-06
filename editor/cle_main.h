#ifndef CLE_MAIN_H
#define CLE_MAIN_H

#include <QtWidgets/QApplication>

#include "cle_memory_inspector.h"
#include "cle_memory_notes.h"
#include "cle_script_editor_block.h"

QApplication *m_Application;
CleMemoryInspector *m_MemoryInspector = nullptr;
CleMemoryNotes *m_MemoryNotes = nullptr;
CleScriptEditorBlock *m_ScriptEditor = nullptr;

#endif
