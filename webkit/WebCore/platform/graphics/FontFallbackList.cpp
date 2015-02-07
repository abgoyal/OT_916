

#include "config.h"
#include "FontFallbackList.h"

#include "Font.h"
#include "FontCache.h"
#include "SegmentedFontData.h"

namespace WebCore {

FontFallbackList::FontFallbackList()
    : m_pageZero(0)
    , m_cachedPrimarySimpleFontData(0)
    , m_fontSelector(0)
    , m_familyIndex(0)
    , m_pitch(UnknownPitch)
    , m_loadingCustomFonts(false)
    , m_generation(fontCache()->generation())
{
}

void FontFallbackList::invalidate(PassRefPtr<FontSelector> fontSelector)
{
    releaseFontData();
    m_fontList.clear();
    m_pageZero = 0;
    m_pages.clear();
    m_cachedPrimarySimpleFontData = 0;
    m_familyIndex = 0;    
    m_pitch = UnknownPitch;
    m_loadingCustomFonts = false;
    m_fontSelector = fontSelector;
    m_generation = fontCache()->generation();
}

void FontFallbackList::releaseFontData()
{
    unsigned numFonts = m_fontList.size();
    for (unsigned i = 0; i < numFonts; ++i) {
        if (!m_fontList[i].second) {
            ASSERT(!m_fontList[i].first->isSegmented());
            fontCache()->releaseFontData(static_cast<const SimpleFontData*>(m_fontList[i].first));
        }
    }
}

void FontFallbackList::determinePitch(const Font* font) const
{
    const FontData* fontData = primaryFontData(font);
    if (!fontData->isSegmented())
        m_pitch = static_cast<const SimpleFontData*>(fontData)->pitch();
    else {
        const SegmentedFontData* segmentedFontData = static_cast<const SegmentedFontData*>(fontData);
        unsigned numRanges = segmentedFontData->numRanges();
        if (numRanges == 1)
            m_pitch = segmentedFontData->rangeAt(0).fontData()->pitch();
        else
            m_pitch = VariablePitch;
    }
}

const FontData* FontFallbackList::fontDataAt(const Font* font, unsigned realizedFontIndex) const
{
    if (realizedFontIndex < m_fontList.size())
        return m_fontList[realizedFontIndex].first; // This fallback font is already in our list.

    // Make sure we're not passing in some crazy value here.
    ASSERT(realizedFontIndex == m_fontList.size());

    if (m_familyIndex == cAllFamiliesScanned)
        return 0;

    // Ask the font cache for the font data.
    // We are obtaining this font for the first time.  We keep track of the families we've looked at before
    // in |m_familyIndex|, so that we never scan the same spot in the list twice.  getFontData will adjust our
    // |m_familyIndex| as it scans for the right font to make.
    ASSERT(fontCache()->generation() == m_generation);
    const FontData* result = fontCache()->getFontData(*font, m_familyIndex, m_fontSelector.get());
    if (result) {
        m_fontList.append(pair<const FontData*, bool>(result, result->isCustomFont()));
        if (result->isLoading())
            m_loadingCustomFonts = true;
    }
    return result;
}

const FontData* FontFallbackList::fontDataForCharacters(const Font* font, const UChar* characters, int length) const
{
    // This method is only called when the primary font does not contain the characters we need.
    // Begin our search at position 1.
    unsigned realizedFontIndex = 1;
    const FontData* fontData = fontDataAt(font, realizedFontIndex);
    while (fontData && !fontData->containsCharacters(characters, length))
        fontData = fontDataAt(font, ++realizedFontIndex);
    
    if (!fontData) {
        ASSERT(fontCache()->generation() == m_generation);
        fontData = fontCache()->getFontDataForCharacters(*font, characters, length);
    }

    return fontData;
}

void FontFallbackList::setPlatformFont(const FontPlatformData& platformData)
{
    m_familyIndex = cAllFamiliesScanned;
    ASSERT(fontCache()->generation() == m_generation);
    const FontData* fontData = fontCache()->getCachedFontData(&platformData);
    m_fontList.append(pair<const FontData*, bool>(fontData, fontData->isCustomFont()));
}

}
