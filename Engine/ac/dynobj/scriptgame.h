//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================
//
// Wrapper around script "GameState" struct, managing access to its variables.
//
//=============================================================================
#ifndef __AGS_EE_STATOBJ__SCRIPTGAME_H
#define __AGS_EE_STATOBJ__SCRIPTGAME_H

#include "ac/dynobj/cc_agsdynamicobject.h"

struct CCScriptGame : public AGSCCStaticObject
{
    int32_t ReadInt32(void *address, intptr_t offset) override;
    void    WriteInt32(void *address, intptr_t offset, int32_t val) override;
};

extern CCScriptGame GameStaticManager;

#endif // __AGS_EE_STATOBJ__SCRIPTGAME_H
