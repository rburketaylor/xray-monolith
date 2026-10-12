#include "stdafx.h"
#include "UIScrollView.h"
#include "UIScrollBar.h"
#include "UIFixedScrollBar.h"
#include "../ui_base.h"
#include "../UICursor.h"
#include "../../xrEngine/xr_input.h"

CUIScrollView::CUIScrollView()
{
	m_rightIndent = 0.0f;
	m_leftIndent = 0.0f;
	m_vertInterval = 0.0f;
	m_upIndent = 0.0f;
	m_downIndent = 0.0f;
	m_flags.zero();
	SetFixedScrollBar(true);
	m_pad = NULL;
	m_VScrollBar = NULL;
	m_visible_rgn.set(-1, -1);
}

CUIScrollView::CUIScrollView(CUIFixedScrollBar* scroll_bar)
{
	m_rightIndent = 0.0f;
	m_leftIndent = 0.0f;
	m_vertInterval = 0.0f;
	m_upIndent = 0.0f;
	m_downIndent = 0.0f;
	m_flags.zero();
	SetFixedScrollBar(true);
	m_pad = NULL;
	m_visible_rgn.set(-1, -1);

	m_VScrollBar = scroll_bar;
	m_VScrollBar->SetAutoDelete(true);
	AttachChild(m_VScrollBar);
	Register(m_VScrollBar);
	AddCallback(m_VScrollBar, SCROLLBAR_VSCROLL, CUIWndCallback::void_function(this, &CUIScrollView::OnScrollV));
}

CUIScrollView::~CUIScrollView()
{
	Clear();
}

void CUIScrollView::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
	CUIWndCallback::OnEvent(pWnd, msg, pData);
	if (CHILD_CHANGED_SIZE == msg && m_pad->IsChild(pWnd))
		m_flags.set(eNeedRecalc,TRUE);
}

void CUIScrollView::ForceUpdate()
{
	m_flags.set(eNeedRecalc,TRUE);
}

void CUIScrollView::InitScrollView()
{
	if (!m_pad)
	{
		m_pad = xr_new<CUIWindow>();
		m_pad->SetAutoDelete(true);
		AttachChild(m_pad);
	}
	m_pad->SetWndPos(Fvector2().set(0, 0));
	if (!m_VScrollBar)
	{
		m_VScrollBar = xr_new<CUIScrollBar>();
		m_VScrollBar->SetAutoDelete(true);
		AttachChild(m_VScrollBar);
		Register(m_VScrollBar);
		AddCallback(m_VScrollBar, SCROLLBAR_VSCROLL, CUIWndCallback::void_function(this, &CUIScrollView::OnScrollV));
	}
	CUIFixedScrollBar* tmp_scroll = smart_cast<CUIFixedScrollBar*>(m_VScrollBar);
	if (tmp_scroll)
		tmp_scroll->InitScrollBar(Fvector2().set(GetWndSize().x, 0.0f), false, *m_scrollbar_profile);
	else
	{
		if (!!m_scrollbar_profile)
			m_VScrollBar->InitScrollBar(Fvector2().set(GetWndSize().x, 0.0f), GetWndSize().y, false,
			                            *m_scrollbar_profile);
		else
			m_VScrollBar->InitScrollBar(Fvector2().set(GetWndSize().x, 0.0f), GetWndSize().y, false);
	}
	Fvector2 sc_pos = {m_VScrollBar->GetWndPos().x - m_VScrollBar->GetWndSize().x, m_VScrollBar->GetWndPos().y};
	m_VScrollBar->SetWndPos(sc_pos);
	m_VScrollBar->SetWindowName("scroll_v");
	m_VScrollBar->SetStepSize(_max(1, iFloor(GetHeight() / 10)));
	m_VScrollBar->SetPageSize(iFloor(GetHeight()));
}

void CUIScrollView::SetScrollBarProfile(LPCSTR profile)
{
	m_scrollbar_profile = profile;
}

void CUIScrollView::AddWindow(CUIWindow* pWnd, bool auto_delete)
{
	if (auto_delete) pWnd->SetAutoDelete(true);

	m_pad->AttachChild(pWnd);
	m_flags.set(eNeedRecalc,TRUE);
}

void CUIScrollView::RemoveWindow(CUIWindow* pWnd)
{
	m_pad->DetachChild(pWnd);
	m_flags.set(eNeedRecalc,TRUE);
}

void CUIScrollView::Clear()
{
	Msg("[invbar] ScrollView::Clear (bar %d, pad_y %.0f)", m_VScrollBar->GetScrollPos(), m_pad->GetWndPos().y);
	m_pad->DetachAll();
	m_flags.set(eNeedRecalc,TRUE);
	ScrollToBegin();
}

Fvector2 CUIScrollView::GetPadSize()
{
	if (m_flags.test(eNeedRecalc))
		RecalcSize();

	return m_pad->GetWndSize();
}

void CUIScrollView::Update()
{
	if (m_flags.test(eNeedRecalc))
		RecalcSize();

	inherited::Update();
}

void CUIScrollView::RecalcSize()
{
	if (!m_pad) return;
	Fvector2 pad_size;
	pad_size.set(0.0f, 0.0f);

	Fvector2 item_pos;
	item_pos.set(m_rightIndent, m_vertInterval + m_upIndent);
	pad_size.y += m_upIndent;
	pad_size.y += m_downIndent;

	xrCriticalSectionGuard guard(m_pad->csUi);
	if (m_sort_function)
	{
		//. m_pad->GetChildWndList().sort(m_sort_function);
		std::sort(m_pad->GetChildWndList().begin(), m_pad->GetChildWndList().end(), m_sort_function);
	}

	if (GetVertFlip())
	{
		for (WINDOW_LIST::reverse_iterator it = m_pad->GetChildWndList().rbegin(); m_pad->GetChildWndList().rend() != it
			; ++it)
		{
			(*it)->SetWndPos(item_pos);
			item_pos.y += (*it)->GetWndSize().y;
			item_pos.y += m_vertInterval;
			pad_size.y += (*it)->GetWndSize().y;
			pad_size.y += m_vertInterval;
			pad_size.x = _max(pad_size.x, (*it)->GetWndSize().x);
		}
	}
	else
	{
		for (WINDOW_LIST_it it = m_pad->GetChildWndList().begin(); m_pad->GetChildWndList().end() != it; ++it)
		{
			(*it)->SetWndPos(item_pos);
			item_pos.y += (*it)->GetWndSize().y;
			item_pos.y += m_vertInterval;
			pad_size.y += (*it)->GetWndSize().y;
			pad_size.y += m_vertInterval;
			pad_size.x = _max(pad_size.x, (*it)->GetWndSize().x);
		}
	};

	m_pad->SetWndSize(pad_size);


	if (m_flags.test(eInverseDir))
		m_pad->SetWndPos(Fvector2().set(m_pad->GetWndPos().x, GetHeight() - m_pad->GetHeight()));

	UpdateScroll();

	m_flags.set(eNeedRecalc, FALSE);
	m_visible_rgn.set(-1, -1);
}

void CUIScrollView::UpdateScroll(bool pad_drag_sync)
{
	Fvector2 w_pos = m_pad->GetWndPos();
	m_VScrollBar->SetHeight(GetHeight());
	int range_max = iFloor(m_pad->GetHeight() * Scroll2ViewV());

	if (pad_drag_sync)
	{
		// The user is dragging the pad itself (OnMouseAction): the pad offset
		// is the live input, so sync the bar from it. This is the only path
		// where the pad may legitimately move the bar.
		m_VScrollBar->SetRange(0, range_max);
		m_VScrollBar->SetScrollPos(iFloor(-w_pos.y));
		return;
	}

	// Layout path (RecalcSize & friends). The bar position is the user's
	// scroll intent and the single source of truth here. Script-driven
	// inventory refreshes Clear() and re-Add() every item; while that storm
	// runs the pad is re-attached at the origin and its height is
	// transiently smaller than the view, so the old code re-derived the
	// position from the transient pad offset (y = 0..16) and clamped it
	// against the collapsed range - silently resetting the bar to 0 and
	// jumping the list mid-scroll. Never let a relayout reduce the position:
	// while the current position exceeds what the (transient) extent allows,
	// keep bar, range and pad exactly as they are; the settled layout pass
	// falls through and re-applies them normally.
	int pos = m_VScrollBar->GetScrollPos();
	int bound = _max(0, range_max - m_VScrollBar->GetPageSize() + 1);
	Msg("[invbar] UpdateScroll pos %d bound %d (pad_h %.0f, range 0..%d) -> %s",
		pos, bound, m_pad->GetHeight(), range_max, (pos > bound) ? "HOLD" : "apply");
	if (pos > bound)
		return;

	m_VScrollBar->SetRange(0, range_max);
	m_VScrollBar->SetScrollPos(pos);
}

float CUIScrollView::Scroll2ViewV()
{
	float h = m_VScrollBar->GetHeight();
	return (h + GetVertIndent()) / h;
}

void CUIScrollView::SetFixedScrollBar(bool b)
{
	m_flags.set(eFixedScrollBar, b);
}

void CUIScrollView::Draw()
{
	if (m_flags.test(eNeedRecalc))
		RecalcSize();

	Frect visible_rect;
	GetAbsoluteRect(visible_rect);
	visible_rect.top += m_upIndent;
	visible_rect.bottom -= m_downIndent;
	UI().PushScissor(visible_rect);

	xrCriticalSectionGuard guard(m_pad->csUi);
	WINDOW_LIST_it it = m_pad->GetChildWndList().begin();
	//	WINDOW_LIST_it it_e					= m_pad->GetChildWndList().end();

	if (!Empty() && m_visible_rgn.x != -1)
	{
		std::advance(it, m_visible_rgn.x);
		for (int idx = m_visible_rgn.x; idx <= m_visible_rgn.y; ++it, ++idx)
		{
			CUIScrollView* sw = smart_cast<CUIScrollView*>(*it);
			VERIFY(sw == NULL);

			if ((*it)->GetVisible())
				(*it)->Draw();
		}
	}
	else
		for (int idx = 0; it != m_pad->GetChildWndList().end(); ++it, ++idx)
		{
			Frect item_rect;
			(*it)->GetAbsoluteRect(item_rect);
			if (visible_rect.intersected(item_rect))
			{
				if (m_visible_rgn.x == -1) //first visible
					m_visible_rgn.x = idx;

				m_visible_rgn.y = idx;

				if ((*it)->GetVisible())
					(*it)->Draw();
			}
			else if (m_visible_rgn.x != -1)
				break;
		}
	UI().PopScissor();

	if (NeedShowScrollBar())
		m_VScrollBar->Draw();
}

bool CUIScrollView::NeedShowScrollBar()
{
	return m_flags.test(eFixedScrollBar) || GetHeight() < m_pad->GetHeight();
}

void CUIScrollView::OnScrollV(CUIWindow*, void*)
{
	int s_pos = m_VScrollBar->GetScrollPos();
	Fvector2 w_pos = m_pad->GetWndPos();
	Msg("[invbar] OnScrollV bar %d -> pad_y %.0f", s_pos, float(-s_pos));
	m_pad->SetWndPos(Fvector2().set(w_pos.x, float(-s_pos)));
	m_visible_rgn.set(-1, -1);
}

bool CUIScrollView::OnMouseAction(float x, float y, EUIMessages mouse_action)
{
	if (inherited::OnMouseAction(x, y, mouse_action)) return true;
	bool res = false;
	int prev_pos = m_VScrollBar->GetScrollPos();
	switch (mouse_action)
	{
	case WINDOW_MOUSE_WHEEL_UP:
		m_VScrollBar->TryScrollDec(true);
		res = true;
		break;
	case WINDOW_MOUSE_WHEEL_DOWN:
		m_VScrollBar->TryScrollInc(true);
		res = true;
		break;
	case WINDOW_MOUSE_MOVE:
		if (pInput->iGetAsyncBtnState(0))
		{
			Fvector2 curr_pad_pos = m_pad->GetWndPos();
			curr_pad_pos.y += GetUICursor().GetCursorPositionDelta().y;

			float max_pos = m_pad->GetHeight() - GetHeight();
			max_pos = _max(0.0f, max_pos);
			clamp(curr_pad_pos.y, -max_pos, 0.0f);
			m_pad->SetWndPos(curr_pad_pos);
			UpdateScroll(true);
			res = true;
		}
		break;
	};
	if (prev_pos != m_VScrollBar->GetScrollPos())
		m_visible_rgn.set(-1, -1);

	return res;
}

int CUIScrollView::GetMinScrollPos()
{
	return m_VScrollBar->GetMinRange();
}

int CUIScrollView::GetMaxScrollPos()
{
	return m_VScrollBar->GetMaxRange();
}

int CUIScrollView::GetCurrentScrollPos()
{
	return m_VScrollBar->GetScrollPos();
}

void CUIScrollView::SetScrollPos(int value)
{
	Msg("[invbar] ScrollView::SetScrollPos req %d (bar was %d)", value, m_VScrollBar->GetScrollPos());
	if (m_flags.test(eNeedRecalc))
		RecalcSize();

	clamp(value, GetMinScrollPos(), GetMaxScrollPos());
	m_VScrollBar->SetScrollPos(value);
	OnScrollV(NULL, NULL);
}

void CUIScrollView::ScrollToBegin()
{
	if (m_flags.test(eNeedRecalc))
		RecalcSize();

	Msg("[invbar] ScrollToBegin was %d (min %d)", m_VScrollBar->GetScrollPos(), m_VScrollBar->GetMinRange());
	m_VScrollBar->SetScrollPos(m_VScrollBar->GetMinRange());
	OnScrollV(NULL, NULL);
}

void CUIScrollView::ScrollToEnd()
{
	if (m_flags.test(eNeedRecalc))
		RecalcSize();

	m_VScrollBar->SetScrollPos(m_VScrollBar->GetMaxRange());
	OnScrollV(NULL, NULL);
}

void CUIScrollView::SetRightIndention(float val)
{
	m_rightIndent = val;
	m_flags.set(eNeedRecalc, TRUE);
}

void CUIScrollView::SetLeftIndention(float val)
{
	m_leftIndent = val;
	m_flags.set(eNeedRecalc, TRUE);
}

void CUIScrollView::SetUpIndention(float val)
{
	m_upIndent = val;
	m_flags.set(eNeedRecalc, TRUE);
}

void CUIScrollView::SetDownIndention(float val)
{
	m_downIndent = val;
	m_flags.set(eNeedRecalc, TRUE);
}

u32 CUIScrollView::GetSize()
{
	return m_pad->GetChildNum();
}

CUIWindow* CUIScrollView::GetItem(u32 idx)
{
	if (m_pad->GetChildNum() <= idx)
		return nullptr;

	xrCriticalSectionGuard guard(m_pad->csUi);
	WINDOW_LIST_it it = m_pad->GetChildWndList().begin();
	std::advance(it, idx);
	return (*it);
}

float CUIScrollView::GetDesiredChildWidth()
{
	if (NeedShowScrollBar())
		return GetWidth() - m_VScrollBar->GetWidth() - m_rightIndent - m_leftIndent;
	else
		return GetWidth() - m_rightIndent - m_leftIndent;
}

float CUIScrollView::GetHorizIndent()
{
	return m_rightIndent + m_leftIndent;
}

float CUIScrollView::GetVertIndent()
{
	return m_upIndent + m_downIndent;
}

void CUIScrollView::SetSelected(CUIWindow* w)
{
	if (!m_flags.test(eItemsSelectabe))
		return;

	xrCriticalSectionGuard guard(m_pad->csUi);
	for (WINDOW_LIST_it it = m_pad->GetChildWndList().begin(); m_pad->GetChildWndList().end() != it; ++it)
	{
		smart_cast<CUISelectable*>(*it)->SetSelected(*it == w);
	}
}

CUIWindow* CUIScrollView::GetSelected()
{
	if (!m_flags.test(eItemsSelectabe))
		return NULL;

	xrCriticalSectionGuard guard(m_pad->csUi);
	for (WINDOW_LIST_it it = m_pad->GetChildWndList().begin(); m_pad->GetChildWndList().end() != it; ++it)
	{
		if (smart_cast<CUISelectable*>(*it)->GetSelected())
			return *it;
	}

	return NULL;
}

void CUIScrollView::UpdateChildrenLenght()
{
	float len = GetDesiredChildWidth();
	xrCriticalSectionGuard guard(m_pad->csUi);
	for (WINDOW_LIST_it it = m_pad->GetChildWndList().begin(); m_pad->GetChildWndList().end() != it; ++it)
	{
		(*it)->SetWidth(len);
	}
}
