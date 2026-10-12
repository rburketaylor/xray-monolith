//=============================================================================
//  Filename:   UIGameLog.h
//	Created by Vitaly 'Mad Max' Maximov, mad-max@gsc-game.kiev.ua
//	Copyright 2005. GSC Game World
//	---------------------------------------------------------------------------
//  Multiplayer game log window
//=============================================================================

#pragma once

#include "UIScrollView.h"

class CUIXml;
class CUIPdaKillMessage;
class CUIPdaMsgListItem;
class CUIStatic;
class CUITextWnd;
struct KillMessageStruct;

class CUIGameLog : public CUIScrollView
{
public:
	CUIGameLog();
	CUITextWnd* AddLogMessage(LPCSTR msg);
	CUIPdaKillMessage* AddLogMessage(KillMessageStruct& msg);
	CUIPdaMsgListItem* AddPdaMessage();
	void AddChatMessage(LPCSTR msg, LPCSTR author);
	virtual void Update();

	void SetTextAtrib(CGameFont* pFont, u32 color);
	u32 GetTextColor() { return txt_color; }

	virtual CUIWindow* ui_cast_window() { return this; }
	virtual CUIScrollView* ui_cast_scroll_view() { return this; }

private:
	WINDOW_LIST toDelList;
	float kill_msg_height;
	u32 txt_color;
	CGameFont* m_pFont;
	// animation expiry runs at ~4 Hz; the trim pass runs on recalcs and
	// whenever the pad moved since the last one (plus a ~4 Hz safety net)
	u32 m_last_anim_pass;
	u32 m_last_trim_pass;
	bool m_trim_ran;
	Fvector2 m_last_pad_pos;
	Fvector2 m_last_abs_pos;
	// the log can be moved without the pad moving relative to it
	Fvector2 m_last_pad_abs_pos;
};
