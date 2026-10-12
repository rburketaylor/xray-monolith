//=============================================================================
//  Filename:   UIGameLog.h
//	Created by Vitaly 'Mad Max' Maximov, mad-max@gsc-game.kiev.ua
//	Copyright 2005. GSC Game World
//	---------------------------------------------------------------------------
//  Multiplayer game log window
//=============================================================================
#include "stdafx.h"
#include "UIGameLog.h"
#include "UIXmlInit.h"
#include "UIPdaMsgListItem.h"
#include "UIPdaKillMessage.h"
#include "UILines.h"

CUIGameLog::CUIGameLog()
{
	kill_msg_height = 20;
	txt_color = 0xff000000;
	m_pFont = NULL;
	m_last_pad_pos.set(0.0f, 0.0f);
	m_last_abs_pos.set(0.0f, 0.0f);
	m_last_pad_abs_pos.set(0.0f, 0.0f);
	m_last_anim_pass = 0;
	m_last_trim_pass = 0;
	m_trim_ran = false;
}

CUITextWnd* CUIGameLog::AddLogMessage(LPCSTR msg)
{
	CUITextWnd* pItem = NULL;
	ADD_TEXT_TO_VIEW3(msg, pItem, this);
	pItem->SetFont(m_pFont);
	pItem->SetTextColor(txt_color);
	pItem->SetColorAnimation("ui_main_msgs_short", LA_ONLYALPHA | LA_TEXTCOLOR, 5000.0f);
	ForceUpdate();
	return pItem;
}

CUIPdaMsgListItem* CUIGameLog::AddPdaMessage()
{
	CUIPdaMsgListItem* pItem = xr_new<CUIPdaMsgListItem>();
	pItem->InitPdaMsgListItem(Fvector2().set(GetDesiredChildWidth(), 10.0f));
	pItem->SetColorAnimation("ui_main_msgs_short", LA_ONLYALPHA | LA_TEXTCOLOR | LA_TEXTURECOLOR);
	AddWindow(pItem, true);

	return pItem;
}

CUIPdaKillMessage* CUIGameLog::AddLogMessage(KillMessageStruct& msg)
{
	CUIPdaKillMessage* pItem = xr_new<CUIPdaKillMessage>();
	pItem->SetWidth(GetDesiredChildWidth());
	pItem->SetHeight(kill_msg_height);
	pItem->Init(msg, m_pFont);
	AddWindow(pItem, true);
	return pItem;
}

void CUIGameLog::AddChatMessage(LPCSTR msg, LPCSTR author)
{
	LPSTR fullLine;
	STRCONCAT(fullLine, author, " ", msg);

	_TrimRight(fullLine);

	CUITextWnd* pItem = xr_new<CUITextWnd>();
	pItem->SetTextComplexMode(true);
	pItem->SetText(fullLine);
	pItem->SetCutWordsMode(true);
	pItem->SetFont(m_pFont);
	pItem->SetTextColor(txt_color);
	pItem->SetColorAnimation("ui_main_msgs_short", LA_ONLYALPHA | LA_TEXTCOLOR, 5000.0f);
	pItem->SetWidth(this->GetDesiredChildWidth());
	pItem->AdjustHeightToText();
	AddWindow(pItem, true);
}

void CUIGameLog::SetTextAtrib(CGameFont* pFont, u32 color)
{
	m_pFont = pFont;
	txt_color = color;
}

// every item is scanned for finished blink animations and again for items
// scrolled out of view; neither result changes at frame rate, so the expiry
// scan runs at ~4 Hz and the trim scan only when the pad may have moved
static const u32 GAME_LOG_UPDATE_PERIOD_MS = 250;

void CUIGameLog::Update()
{
	CUIScrollView::Update();

	const u32 now = Device.dwTimeGlobal;

	// the pad moves when the log is scrolled (wheel, bar, drag); the window
	// itself moves with the HUD. Either invalidates the trimmed rects.
	Fvector2 abs_pos;
	GetAbsolutePos(abs_pos);
	Fvector2 pad_abs_pos;
	m_pad->GetAbsolutePos(pad_abs_pos);

	const bool pad_moved = !m_trim_ran
		|| !m_last_pad_pos.similar(m_pad->GetWndPos())
		|| !m_last_abs_pos.similar(abs_pos)
		|| !m_last_pad_abs_pos.similar(pad_abs_pos);

	const bool anim_pass_due = (now - m_last_anim_pass) >= GAME_LOG_UPDATE_PERIOD_MS;
	const bool trim_pass_due = (now - m_last_trim_pass) >= GAME_LOG_UPDATE_PERIOD_MS;
	const bool trim = m_flags.test(eNeedRecalc) || pad_moved || trim_pass_due;

	if (!anim_pass_due && !trim)
		return;

	xrCriticalSectionGuard guard(m_pad->csUi);
	WINDOW_LIST_it it = m_pad->GetChildWndList().begin();
	WINDOW_LIST_it it_e = m_pad->GetChildWndList().end();

	if (anim_pass_due)
	{
		m_last_anim_pass = now;
		toDelList.clear();

		for (; it != it_e; ++it)
		{
			CUILightAnimColorConroller* pItem = smart_cast<CUILightAnimColorConroller*>(*it);

			if (!pItem->IsColorAnimationPresent())
				toDelList.push_back(*it);
		}

		// Delete elements
		it_e = toDelList.end();

		for (it = toDelList.begin(); it != it_e; ++it)
			RemoveWindow(*it);
	}

	if (trim)
	{
		m_trim_ran = true;
		m_last_trim_pass = now;
		m_last_pad_pos = m_pad->GetWndPos();
		GetAbsolutePos(abs_pos);
		m_last_abs_pos = abs_pos;
		m_pad->GetAbsolutePos(pad_abs_pos);
		m_last_pad_abs_pos = pad_abs_pos;

		// REMOVE INVISIBLE AND PART VISIBLE ITEMS
		if (m_flags.test(eNeedRecalc))
			RecalcSize();

		toDelList.clear();
		Frect visible_rect;
		GetAbsoluteRect(visible_rect);
		it_e = m_pad->GetChildWndList().end();
		for (it = m_pad->GetChildWndList().begin(); it != it_e; ++it)
		{
			Frect r;
			(*it)->GetAbsoluteRect(r);
			r.shrink(3.0f, 3.0f);

			if (! (visible_rect.in(r.x1, r.y1) && visible_rect.in(r.x2, r.y1) && visible_rect.in(r.x1, r.y2) && visible_rect
				.in(r.x2, r.y2)))
				toDelList.push_back(*it);
		}

		// Delete elements
		it_e = toDelList.end();
		for (it = toDelList.begin(); it != it_e; ++it)
			RemoveWindow(*it);

		if (m_flags.test(eNeedRecalc))
			RecalcSize();
	}
}
