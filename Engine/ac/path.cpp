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

#include "ac/gamesetupstruct.h"
#include "ac/path.h"

extern GameSetupStruct game;

void convert_move_path_to_high_res(MoveList *ml)
{
    const int mul = game.GetUpscaleMult();
    ml->fromx *= mul;
    ml->fromy *= mul;
    ml->lastx *= mul;
    ml->lasty *= mul;

    for (int i = 0; i < ml->numstage; i++)
    {
        short lowPart = (ml->pos[i] & 0x0000ffff) * mul;
        short highPart = ((ml->pos[i] >> 16) & 0x0000ffff) * mul;
        ml->pos[i] = ((int)highPart << 16) | (lowPart & 0x0000ffff);

        ml->xpermove[i] *= mul;
        ml->ypermove[i] *= mul;
    }
}
