#include "stdafx.h"
#pragma hdrstop

XRCORE_API BOOL g_bEnableStatGather = FALSE;

CStatTimer::CStatTimer()
{
	accum = 0;
	result = 0.f;
	count = 0;
}

void CStatTimer::FrameStart()
{
	accum = 0;
	count = 0;
}

void CStatTimer::FrameEnd()
{
	float _time = 1000.f * float(double(accum) / double(CPU::qpc_freq));
	if (_time > result) result = _time;
	else result = 0.99f * result + 0.01f * _time;
}

XRCORE_API pauseMngr& g_pauseMngr()
{
	static pauseMngr manager;
	return manager;
}

pauseMngr::pauseMngr() : m_paused(false)
{
	m_timers.reserve(3);
}

void pauseMngr::Pause(const bool b)
{
	if (m_paused == b)return;

	xr_vector<CTimer_paused*>::iterator it = m_timers.begin();
	for (; it != m_timers.end(); ++it)
		(*it)->Pause(b);

	m_paused = b;
}

void pauseMngr::Register(CTimer_paused& t)
{
	m_timers.push_back(&t);
}

void pauseMngr::UnRegister(CTimer_paused& t)
{
	const auto it = std::find(m_timers.cbegin(), m_timers.cend(), &t);
	if (it != m_timers.end())
		m_timers.erase_fast(it);
}

int g_perf_timers = 0;

static const float perf_timer_min_ms = 1.0f;

CPerfScopeTimer::CPerfScopeTimer(const char* name) : m_name(name)
{
	m_start = g_perf_timers ? CPU::QPC() : 0;
}

CPerfScopeTimer::~CPerfScopeTimer()
{
	if (!g_perf_timers) return;

	u64 elapsed = CPU::QPC() - m_start;
	float ms = 1000.f * float(double(elapsed) / double(CPU::qpc_freq));
	if (ms >= perf_timer_min_ms)
		Msg("[perf] %s %.2f ms", m_name, ms);
}

CPerfAccumScope::CPerfAccumScope(CPerfAccumTimer& owner, u32 frame) : m_owner(owner), m_frame(frame)
{
	m_start = g_perf_timers ? CPU::QPC() : 0;
}

CPerfAccumScope::~CPerfAccumScope()
{
	if (!g_perf_timers) return;

	if (m_owner.m_frame != m_frame && m_owner.m_hits > 0)
	{
		if (m_owner.m_ms >= perf_timer_min_ms)
			Msg("[perf] %s %.2f ms (%u calls)", m_owner.m_name, m_owner.m_ms, m_owner.m_hits);
		m_owner.m_hits = 0;
		m_owner.m_ms = 0.f;
	}

	u64 elapsed = CPU::QPC() - m_start;
	m_owner.m_ms += 1000.f * float(double(elapsed) / double(CPU::qpc_freq));
	m_owner.m_hits++;
	m_owner.m_frame = m_frame;
}
