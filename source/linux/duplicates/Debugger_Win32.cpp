#include "StdAfx.h"
#include "Debug.h"

Update_t CmdConfigFont(int /* nArgs */)
{
  return UPDATE_NOTHING;
}

Update_t CmdConfigGetFont(int /* nArgs */)
{
  return UPDATE_NOTHING;
}

void ProcessClipboardCommands()
{
}

void FontsDestroy()
{
}

// Only for FONT_DISASM_DEFAULT !
static void _UpdateWindowFontHeights(int nFontHeight)
{
	if (nFontHeight)
	{
		int nConsoleTopY = GetConsoleTopPixels(g_nConsoleDisplayLines);

		int nHeight = 0;

		if (g_iFontSpacing == FONT_SPACING_CLASSIC)
		{
			nHeight = nFontHeight + 1;
			g_nDisasmDisplayLines = nConsoleTopY / nHeight;
		}
		else
			if (g_iFontSpacing == FONT_SPACING_CLEAN)
			{
				nHeight = nFontHeight;
				g_nDisasmDisplayLines = nConsoleTopY / nHeight;
			}
			else
				if (g_iFontSpacing == FONT_SPACING_COMPRESSED)
				{
					nHeight = nFontHeight - 1;
					g_nDisasmDisplayLines = (nConsoleTopY + nHeight) / nHeight; // Ceil()
				}

		g_aFontConfig[FONT_DISASM_DEFAULT]._nLineHeight = nHeight;

		//		int nHeightOptimal = (nHeight0 + nHeight1) / 2;
		//		int nLinesOptimal = nConsoleTopY / nHeightOptimal;
		//		g_nDisasmDisplayLines = nLinesOptimal;

		WindowUpdateSizes();
	}
}

void FontsInitialize()
{
	for (int iFont = 0; iFont < NUM_FONTS; iFont++ )
	{
		g_aFontConfig[ iFont ]._hFont = NULL;
		g_aFontConfig[ iFont ]._nFontHeight   = CONSOLE_FONT_HEIGHT;
		g_aFontConfig[ iFont ]._nFontWidthAvg = CONSOLE_FONT_WIDTH;
		g_aFontConfig[ iFont ]._nFontWidthMax = CONSOLE_FONT_WIDTH;
		g_aFontConfig[ iFont ]._nLineHeight   = CONSOLE_FONT_HEIGHT;
	}

	_UpdateWindowFontHeights( g_aFontConfig[ FONT_DISASM_DEFAULT ]._nFontHeight );
}
