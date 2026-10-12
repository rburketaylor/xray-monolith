#pragma once

#include "GameTaskDefs.h"
#include "object_interfaces.h"

class CGameTaskWrapper;
class CGameTask;
class CMapLocation;

class CGameTaskManager
{
	CGameTaskWrapper* m_gametasks_wrapper;
	vGameTasks* m_gametasks;
	// scratch snapshot of the task list, reused by UpdateTasks() so it does not
	// have to copy the vector onto the stack (or alloca a buffer) every frame
	xr_vector<SGameTaskKey> m_tasks_scratch;

	enum { eChanged = (1 << 0), };

	Flags8 m_flags;
	u32 m_actual_frame;
	//	bumped on every change that can alter HasGameTask(ml, only_inprocess)
	//	results (task added, task state changed, map location detached)
	u32 m_tasks_generation;
protected:
	void UpdateActiveTask();
public:

	CGameTaskManager();
	~CGameTaskManager();

	vGameTasks& GetGameTasks();
	CGameTask* HasGameTask(const CMapLocation* ml, bool only_inprocess);
	CGameTask* HasGameTask(const shared_str& id, bool only_inprocess);
	CGameTask* GiveGameTaskToActor(CGameTask* t, u32 timeToComplete, bool bCheckExisting, u32 timer_ttl);
	void SetTaskState(const shared_str& id, ETaskState state);
	void SetTaskState(CGameTask* t, ETaskState state);
	//	per-frame task-lookup cache invalidator (see CMapLocation::task_in_progress)
	u32 GetTasksGeneration() const { return m_tasks_generation; }

	void RPC_UpdateTaskName();

	void __stdcall UpdateTasks();

	CGameTask* ActiveTask();
	//	void					SetActiveTask					(const shared_str& id);
	void SetActiveTask(CGameTask* task);
	u32 ActualFrame() const { return m_actual_frame; }

	CGameTask* IterateGet(CGameTask* t, ETaskState state, bool bForward);
	u32 GetTaskIndex(CGameTask* t, ETaskState state);
	u32 GetTaskCount(ETaskState state);
	void MapLocationRelcase(CMapLocation* ml);

	void ResetStorage() { m_gametasks = NULL; };
	void DumpTasks();
};
