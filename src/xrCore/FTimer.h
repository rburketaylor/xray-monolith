#ifndef FTimerH
#define FTimerH
#pragma once

class CTimer_paused;

class XRCORE_API pauseMngr
{
	xr_vector<CTimer_paused*> m_timers;
	bool m_paused;
public:
	pauseMngr();
	bool Paused() { return m_paused; };
	void Pause(bool b);
	void Register(CTimer_paused& t);
	void UnRegister(CTimer_paused& t);
};

extern XRCORE_API pauseMngr& g_pauseMngr();

class XRCORE_API CTimerBase
{
protected:
	u64 qwStartTime;
	u64 qwPausedTime;
	u64 qwPauseAccum;
	bool bPause;
public:
	CTimerBase() : qwStartTime(0), qwPausedTime(0), qwPauseAccum(0), bPause(false)
	{
	}

	ICF void Start()
	{
		if (bPause) return;
		qwStartTime = CPU::QPC() - qwPauseAccum;
	}

	ICF u64 GetElapsed_ticks() const
	{
		if (bPause) return qwPausedTime;
		else return CPU::QPC() - qwStartTime - CPU::qpc_overhead - qwPauseAccum;
	}

	IC u32 GetElapsed_ms() const { return u32(GetElapsed_ticks() * u64(1000) / CPU::qpc_freq); }
	IC float GetElapsed_sec() const
	{
#ifndef _EDITOR
		FPU::m64r();
#endif
		float _result = float(double(GetElapsed_ticks()) / double(CPU::qpc_freq));
#ifndef _EDITOR
		FPU::m24r();
#endif
		return _result;
	}

	IC void Dump() const
	{
		Msg("* Elapsed time (sec): %f", GetElapsed_sec());
	}
};

class XRCORE_API CTimer : public CTimerBase
{
private:
	typedef CTimerBase inherited;

private:
	float m_time_factor;
	u64 m_real_ticks;
	u64 m_ticks;

private:
	IC u64 GetElapsed_ticks(const u64& current_ticks) const
	{
		u64 delta = current_ticks - m_real_ticks;
		double delta_d = (double)delta;
		double time_factor_d = time_factor();
		double time = delta_d * time_factor_d + .5;
		u64 result = (u64)time;
		return (m_ticks + result);
	}

public:
	IC CTimer() : m_time_factor(1.f), m_real_ticks(0), m_ticks(0)
	{
	}

	ICF void Start()
	{
		if (bPause)
			return;

		inherited::Start();

		m_real_ticks = 0;
		m_ticks = 0;
	}

	IC const float& time_factor() const
	{
		return (m_time_factor);
	}

	IC void time_factor(const float& time_factor)
	{
		u64 current = inherited::GetElapsed_ticks();
		m_ticks = GetElapsed_ticks(current);
		m_real_ticks = current;
		m_time_factor = time_factor;
	}

	IC u64 GetElapsed_ticks() const
	{
#ifndef _EDITOR
		FPU::m64r();
#endif // _EDITOR

		u64 result = GetElapsed_ticks(inherited::GetElapsed_ticks());

#ifndef _EDITOR
		FPU::m24r();
#endif // _EDITOR

		return (result);
	}

	IC u32 GetElapsed_ms() const
	{
		return (u32(GetElapsed_ticks() * u64(1000) / CPU::qpc_freq));
	}

	IC float GetElapsed_ms_f() const {
		return float(double(GetElapsed_ticks() * u64(1000)) / CPU::qpc_freq);
	}

	IC float GetElapsed_sec() const
	{
#ifndef _EDITOR
		FPU::m64r();
#endif
		float result = float(double(GetElapsed_ticks()) / double(CPU::qpc_freq));
#ifndef _EDITOR
		FPU::m24r();
#endif
		return (result);
	}

	IC void Dump() const
	{
		Msg("* Elapsed time (sec): %f", GetElapsed_sec());
	}
};

class XRCORE_API CTimer_paused_ex : public CTimer
{
	u64 save_clock;
public:
	CTimer_paused_ex()
	{
	}

	virtual ~CTimer_paused_ex()
	{
	}

	IC bool Paused() const { return bPause; }
	IC void Pause(const bool b)
	{
		if (bPause == b) return;

		u64 _current = CPU::QPC() - CPU::qpc_overhead;
		if (b)
		{
			save_clock = _current;
			qwPausedTime = CTimerBase::GetElapsed_ticks();
		}
		else
		{
			qwPauseAccum += _current - save_clock;
		}
		bPause = b;
	}
};

class XRCORE_API CTimer_paused : public CTimer_paused_ex
{
public:
	CTimer_paused() { g_pauseMngr().Register(*this); }
	virtual ~CTimer_paused() { g_pauseMngr().UnRegister(*this); }
};

extern XRCORE_API BOOL g_bEnableStatGather;

class XRCORE_API CStatTimer
{
public:
	CTimer T;
	u64 accum;
	float result;
	u32 count;
public:
	CStatTimer();
	void FrameStart();
	void FrameEnd();

	ICF void Begin()
	{
		if (!g_bEnableStatGather) return;
		count++;
		T.Start();
	}

	ICF void End()
	{
		if (!g_bEnableStatGather) return;
		accum += T.GetElapsed_ticks();
	}

	ICF u64 GetElapsed_ticks() const { return accum; }

	IC u32 GetElapsed_ms() const { return u32(GetElapsed_ticks() * u64(1000) / CPU::qpc_freq); }
	IC float GetElapsed_sec() const
	{
#ifndef _EDITOR
		FPU::m64r();
#endif
		float _result = float(double(GetElapsed_ticks()) / double(CPU::qpc_freq));
#ifndef _EDITOR
		FPU::m24r();
#endif
		return _result;
	}
};

// Gated hot-path timers, controlled by the "perf_timers" console variable (default 0).
// Scope timer logs "[perf] <name> <ms>" on destruction when enabled and the elapsed
// time is at least perf_timer_min_ms. Accumulated timers sum several calls of one
// name within a frame and log the frame total when the frame id advances.
extern XRCORE_API int g_perf_timers;

class XRCORE_API CPerfScopeTimer
{
	const char* m_name;
	u64 m_start;

public:
	CPerfScopeTimer(const char* name);
	~CPerfScopeTimer();
};

class XRCORE_API CPerfAccumTimer
{
	friend class CPerfAccumScope;
	const char* m_name;
	u32 m_frame;
	u32 m_hits;
	float m_ms;

public:
	CPerfAccumTimer(const char* name) : m_name(name), m_frame(0), m_hits(0), m_ms(0.f) { }
};

// Feeds one measured scope into a CPerfAccumTimer; flushes the previous frame's total
// when the frame id passed to the constructor differs from the accumulator's frame.
class XRCORE_API CPerfAccumScope
{
	CPerfAccumTimer& m_owner;
	u32 m_frame;
	u64 m_start;

public:
	CPerfAccumScope(CPerfAccumTimer& owner, u32 frame);
	~CPerfAccumScope();
};

#endif // FTimerH
