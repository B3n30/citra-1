// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "ui_settings.h"

namespace UISettings {

Shortcut::Shortcut(const char* name, const char* group, ContextualShortcut shortcut)
    : name(name), group(group), shortcut(shortcut) {}

Shortcut::Shortcut(const QString name, const QString group, ContextualShortcut shortcut)
    : name(name), group(group), shortcut(shortcut) {}

Values values = {};
} // namespace UISettings
