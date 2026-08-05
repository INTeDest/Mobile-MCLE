#include "UIBitmapFont.h"

#include <cstring>

#include "app/common/Iggy/include/iggy.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif
#include "UIFontData.h"
#include "app/common/Iggy/include/rrCore.h"
#include "minecraft/client/BufferedImage.h"

/////////////////////////////
// UI Abstract Bitmap Font //
/////////////////////////////

UIAbstractBitmapFont::~UIAbstractBitmapFont() {
    if (m_registered)
        IggyFontRemoveUTF8(m_fontname.c_str(), -1, IGGY_FONTFLAG_none);
    delete m_bitmapFontProvider;
}

UIAbstractBitmapFont::UIAbstractBitmapFont(const std::string& fontname) {
    m_fontname = fontname;

    m_registered = false;

    m_bitmapFontProvider = new IggyBitmapFontProvider();
    
    // КРИТИЧЕСКИЙ ФИКС: Обязательное зануление памяти.
    // Предотвращает попытки Iggy вызвать мусорные указатели на функции, 
    // которые приводят к SEGV_ACCERR!
    memset(m_bitmapFontProvider, 0, sizeof(IggyBitmapFontProvider));

    m_bitmapFontProvider->get_font_metrics =
        &UIAbstractBitmapFont::GetFontMetrics_Callback;
    m_bitmapFontProvider->get_glyph_for_codepoint =
        &UIAbstractBitmapFont::GetCodepointGlyph_Callback;
    m_bitmapFontProvider->get_glyph_metrics =
        &UIAbstractBitmapFont::GetGlyphMetrics_Callback;
    m_bitmapFontProvider->is_empty =
        &UIAbstractBitmapFont::IsGlyphEmpty_Callback;
    m_bitmapFontProvider->get_kerning =
        &UIAbstractBitmapFont::GetKerningForGlyphPair_Callback;
    m_bitmapFontProvider->can_bitmap =
        &UIAbstractBitmapFont::CanProvideBitmap_Callback;
    m_bitmapFontProvider->get_bitmap =
        &UIAbstractBitmapFont::GetGlyphBitmap_Callback;
    m_bitmapFontProvider->free_bitmap =
        &UIAbstractBitmapFont::FreeGlyphBitmap_Callback;
    m_bitmapFontProvider->userdata = this;
}

void UIAbstractBitmapFont::registerFont() {
    if (!m_registered) {
        // 4J-JEV: These only need registering the once when we first use this
        // font in Iggy.
        m_bitmapFontProvider->num_glyphs = m_numGlyphs;
        IggyFontInstallBitmapUTF8(m_bitmapFontProvider, m_fontname.c_str(), -1,
                                  IGGY_FONTFLAG_none);
        m_registered = true;
    }

    // 4J-JEV: Reset the font redirect to these fonts (we must do this everytime
    // in-case we switched away elsewhere).
    IggyFontSetIndirectUTF8(m_fontname.c_str(), -1, IGGY_FONTFLAG_all,
                            m_fontname.c_str(), -1, IGGY_FONTFLAG_none);
}

IggyFontMetrics* RADLINK UIAbstractBitmapFont::GetFontMetrics_Callback(
    void* user_context, IggyFontMetrics* metrics) {
    return ((UIAbstractBitmapFont*)user_context)->GetFontMetrics(metrics);
}

S32 RADLINK UIAbstractBitmapFont::GetCodepointGlyph_Callback(void* user_context,
                                                             U32 codepoint) {
    return ((UIAbstractBitmapFont*)user_context)->GetCodepointGlyph(codepoint);
}

IggyGlyphMetrics* RADLINK UIAbstractBitmapFont::GetGlyphMetrics_Callback(
    void* user_context, S32 glyph, IggyGlyphMetrics* metrics) {
    return ((UIAbstractBitmapFont*)user_context)
        ->GetGlyphMetrics(glyph, metrics);
}

rrbool RADLINK UIAbstractBitmapFont::IsGlyphEmpty_Callback(void* user_context,
                                                           S32 glyph) {
    return ((UIAbstractBitmapFont*)user_context)->IsGlyphEmpty(glyph);
}

F32 RADLINK UIAbstractBitmapFont::GetKerningForGlyphPair_Callback(
    void* user_context, S32 first_glyph, S32 second_glyph) {
    return ((UIAbstractBitmapFont*)user_context)
        ->GetKerningForGlyphPair(first_glyph, second_glyph);
}

rrbool RADLINK UIAbstractBitmapFont::CanProvideBitmap_Callback(
    void* user_context, S32 glyph, F32 pixel_scale) {
    return ((UIAbstractBitmapFont*)user_context)
        ->CanProvideBitmap(glyph, pixel_scale);
}

rrbool RADLINK UIAbstractBitmapFont::GetGlyphBitmap_Callback(
    void* user_context, S32 glyph, F32 pixel_scale,
    IggyBitmapCharacter* bitmap) {
    return ((UIAbstractBitmapFont*)user_context)
        ->GetGlyphBitmap(glyph, pixel_scale, bitmap);
}

void RADLINK UIAbstractBitmapFont::FreeGlyphBitmap_Callback(
    void* user_context, S32 glyph, F32 pixel_scale,
    IggyBitmapCharacter* bitmap) {
    return ((UIAbstractBitmapFont*)user_context)
        ->FreeGlyphBitmap(glyph, pixel_scale, bitmap);
}

UIBitmapFont::UIBitmapFont(SFontData& sfontdata)
    : UIAbstractBitmapFont(sfontdata.m_strFontName) {
    m_numGlyphs = sfontdata.m_uiGlyphCount;

    BufferedImage bimg(sfontdata.m_wstrFilename);
    int* bimgData = bimg.getData();

    m_cFontData = new CFontData(sfontdata, bimgData);

    // delete [] bimgData;
}

UIBitmapFont::~UIBitmapFont() { m_cFontData->release(); }

// Callback function type for returning vertical font metrics
IggyFontMetrics* UIBitmapFont::GetFontMetrics(IggyFontMetrics* metrics) {
    metrics->ascent = m_cFontData->getFontData()->m_fAscent;
    metrics->descent = m_cFontData->getFontData()->m_fDescent;

    metrics->average_glyph_width_for_tab_stops = 8.0f;
    metrics->largest_glyph_bbox_y1 = metrics->descent;

    return metrics;
}

// Callback function type for mapping 32-bit unicode code point to internal font
// glyph number; use IGGY_GLYPH_INVALID to mean "invalid character"
S32 UIBitmapFont::GetCodepointGlyph(U32 codepoint) {
    // 4J-JEV: Change "right single quotation marks" to apostrophies.
    if (codepoint == 0x2019) codepoint = 0x27;

    return m_cFontData->getGlyphId(codepoint);
}

// Callback function type for returning horizontal metrics for each glyph
IggyGlyphMetrics* UIBitmapFont::GetGlyphMetrics(S32 glyph,
                                                IggyGlyphMetrics* metrics) {
    float glyphAdvance = m_cFontData->getAdvance(glyph);

    metrics->x0 = 0.0f;

    if (m_cFontData->glyphIsWhitespace(glyph))
        metrics->x1 = 0.0f;
    else
        metrics->x1 = glyphAdvance;

    // The next Glyph just starts right after this one.
    metrics->advance = glyphAdvance;

    metrics->y0 = 0.0f;
    metrics->y1 = 1.0f;

    return metrics;
}

// Callback function type that should return true iff the glyph has no visible
// elements
rrbool UIBitmapFont::IsGlyphEmpty(S32 glyph) {
    if (m_cFontData->glyphIsWhitespace(glyph)) return true;
    return false;
}

// Callback function type for returning the kerning amount for a given pair of
// glyphs
F32 UIBitmapFont::GetKerningForGlyphPair(S32 first_glyph, S32 second_glyph) {
    return 0.0f;
}

// Callback function type used for reporting whether a bitmap supports a given
// glyph at the given scale
rrbool UIBitmapFont::CanProvideBitmap(S32 glyph, F32 pixel_scale) {
    return true;
}

rrbool UIBitmapFont::GetGlyphBitmap(S32 glyph, F32 pixel_scale,
                                    IggyBitmapCharacter* bitmap) {
    int row = 0, col = 0;
    m_cFontData->getPos(glyph, row, col);

    // Skip to glyph start.
    bitmap->pixels_one_per_byte = m_cFontData->topLeftPixel(row, col);

    // Choose a reasonable glyph scale.
    float glyphScale = 1.0f,
          truePixelScale = 1.0f / m_cFontData->getFontData()->m_fAdvPerPixel;
    F32 targetPixelScale = pixel_scale;

    while ((0.5f + glyphScale) * truePixelScale < targetPixelScale)
        glyphScale++;

    bitmap->width_in_pixels = m_cFontData->getFontData()->m_uiGlyphWidth;
    bitmap->height_in_pixels = m_cFontData->getFontData()->m_uiGlyphHeight;

    bitmap->top_left_x = 0;
    bitmap->top_left_y = -((S32)m_cFontData->getFontData()->m_uiGlyphHeight) *
                         m_cFontData->getFontData()->m_fAscent;

    bitmap->oversample = 0;
    bitmap->point_sample = true;

    bitmap->pixel_scale_correct = pixel_scale / glyphScale;
    bitmap->pixel_scale_max = 99.0f;
    bitmap->pixel_scale_min = 0.0f;

    bitmap->stride_in_bytes = m_cFontData->getFontData()->m_uiGlyphMapX;
    bitmap->user_context_for_free = nullptr;

    return true;
}

// Callback function type for freeing a bitmap shape returned by GetGlyphBitmap
void UIBitmapFont::FreeGlyphBitmap(S32 glyph, F32 pixel_scale,
                                   IggyBitmapCharacter* bitmap) {
}