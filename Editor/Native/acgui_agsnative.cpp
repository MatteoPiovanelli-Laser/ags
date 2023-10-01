//
// Implementation from acgui.h and acgui.cpp specific to AGS.Native library
//

#pragma unmanaged

#include "ac/gamesetupstruct.h"
#include "font/fonts.h"
#include "gui/guimain.h"
#include "gui/guibutton.h"
#include "gui/guiinv.h"
#include "gui/guilabel.h"
#include "gui/guilistbox.h"
#include "gui/guitextbox.h"
#include "util/string_utils.h"

extern GameSetupStruct thisgame;

bool AGS::Common::GUIMain::HasAlphaChannel() const
{
    if (this->BgImage > 0)
    {
        // alpha state depends on background image
        return is_sprite_alpha(this->BgImage);
    }
    if (this->BgColor > 0)
    {
        // not alpha transparent if there is a background color
        return false;
    }
    // transparent background, enable alpha blending
    return thisgame.color_depth * 8 >= 24;
}

//=============================================================================
// AGS.Native-specific implementation split out of acgui.h
//=============================================================================

void wouttext_outline(Common::Bitmap *ds, int xxp, int yyp, int usingfont, color_t text_color, const char *texx)
{
  wouttextxy(ds, xxp, yyp, usingfont, text_color, texx);
}

//=============================================================================
// AGS.Native-specific implementation split out of acgui.cpp
//=============================================================================

int final_col_dep = 32;

bool is_sprite_alpha(int spr)
{
  return false;
}

void set_eip_guiobj(int eip)
{
  // do nothing
}

int get_eip_guiobj()
{
  return 0;
}

int get_text_width_outlined(const char *tex, int font)
{
  return get_text_width(tex, font);
}

namespace AGS
{
namespace Common
{

String GUI::TransformTextForDrawing(const String &text, bool /*translate*/, bool /*apply_direction*/)
{
    return text;
}

size_t GUI::SplitLinesForDrawing(const char *text, bool /*apply_direction*/, SplitLines &lines, int font, int width, size_t max_lines)
{
    return split_lines(text, lines, width, font, max_lines);
}

void GUIObject::MarkChanged()
{
    // do nothing: in Editor "guis" array is not even guaranteed to be filled!
}

void GUIObject::MarkParentChanged()
{
    // do nothing: in Editor "guis" array is not even guaranteed to be filled!
}

void GUIObject::MarkPositionChanged(bool)
{
    // do nothing: in Editor "guis" array is not even guaranteed to be filled!
}

void GUIObject::MarkStateChanged(bool, bool)
{
    // do nothing: in Editor "guis" array is not even guaranteed to be filled!
}

int GUILabel::PrepareTextToDraw()
{
    _textToDraw = Text;
    return GUI::SplitLinesForDrawing(_textToDraw.GetCStr(), false, Lines, Font, _width);
}

void GUITextBox::DrawTextBoxContents(Bitmap *ds, int x, int y, color_t text_color)
{
    // print something fake so we can see what it looks like
    wouttext_outline(ds, x + 2, y + 2, Font, text_color, "Text Box Contents");
}

void GUIListBox::PrepareTextToDraw(const String &text)
{
    _textToDraw = text;
}

bool GUIInvWindow::HasAlphaChannel() const
{
    return false; // don't do alpha in the editor
}

void GUIInvWindow::Draw(Bitmap *ds, int x, int y)
{
    color_t draw_color = ds->GetCompatibleColor(15);
    ds->DrawRect(RectWH(x, y, _width, _height), draw_color);
}

void GUIButton::PrepareTextToDraw()
{
    _textToDraw = _text;
}

} // namespace Common
} // namespace AGS
