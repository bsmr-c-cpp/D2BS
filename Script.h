#pragma once

#include <windows.h>
#include <string>
#include <map>
#include <list>

#include "js32.h"
#include "JSGlobalClasses.h"
#include "AutoRoot.h"
#include "JSUnit.h"

enum ScriptState {
	InGame,
	OutOfGame,
	Command
};

class Script;

typedef std::map<std::string, bool> IncludeList;
typedef std::list<AutoRoot*> FunctionList;
typedef std::map<std::string, FunctionList> FunctionMap;
typedef std::list<Script*> ScriptList;

class Event {
public:
	Event(Script* _owner, JSObject* _object, AutoRoot** _argv, uintN _argc)
	{
		owner = _owner;
		object = _object;
		argv = _argv;
		argc = _argc;
		dead = false;
	}

	Script* owner;
	JSObject* object;
	FunctionList functions;
	AutoRoot** argv;
	uintN argc;
	bool dead;
};

class Script
{
private:
	std::string fileName;
	int execCount;
	ScriptState scriptState;
	JSContext* context;
	JSScript* script;
	myUnit* me;
	uintN argc;
	jsval* argv;

	JSObject *globalObject, *scriptObject;
	bool isLocked, isPaused, isReallyPaused, isAborted;

	IncludeList includes, inProgress;
	FunctionMap functions;
	HANDLE threadHandle;
	DWORD threadId;
	CRITICAL_SECTION lock;

	Script(const char* file, ScriptState state, uintN argc = 0, jsval* argv = NULL);
	Script(const Script&);
	Script& operator=(const Script&);
	~Script(void);

public:
	friend class ScriptEngine;
	
	void Run(void);
	void Join(void);
	void Pause(void);
	void Resume(void);
	bool IsPaused(void);
	bool BeginThread(LPTHREAD_START_ROUTINE ThreadFunc);
	void RunCommand(const char* command);
	inline void SetPauseState(bool reallyPaused) { isReallyPaused = reallyPaused; }
	inline bool IsReallyPaused(void) { return isReallyPaused; }
	void Stop(bool force = false, bool reallyForce = false);

	inline const char* GetFilename(void) { return fileName.c_str(); }
	const char* GetShortFilename(void);
	inline JSContext* GetContext(void) { return context; }
	inline JSObject* GetGlobalObject(void) { return globalObject; }
	inline JSObject* GetScriptObject(void) { return scriptObject; }
	inline ScriptState GetState(void) { return scriptState; }
	int GetExecutionCount(void);
	DWORD GetThreadId(void);
	// UGLY HACK to fix up the player gid on game join for cached scripts/oog scripts
	void UpdatePlayerGid(void);
	// Hack. Include from console needs to run on the RunCommandThread / cx. 
	//		 a better solution may be to keep a list of threadId / cx and have a GetCurrentThreadCX()   
	inline void SetContext(JSContext* cx) { context = cx;}
	bool IsRunning(void);
	bool IsAborted(void);

	bool IsIncluded(const char* file);
	bool Include(const char* file);

	bool IsListenerRegistered(const char* evtName);
	void RegisterEvent(const char* evtName, jsval evtFunc);
	bool IsRegisteredEvent(const char* evtName, jsval evtFunc);
	void UnregisterEvent(const char* evtName, jsval evtFunc);
	void ClearEvent(const char* evtName);
	void ClearAllEvents(void);

	void ExecEventAsync(char* evtName, uintN argc, AutoRoot** argv);
	bool ExecEvent(char* evtName, uintN argc, AutoRoot** argv);
};

struct RUNCOMMANDSTRUCT {
	Script* script;
	const char* command;
};

DWORD WINAPI RunCommandThread(void* data);
DWORD WINAPI ScriptThread(void* data);
DWORD WINAPI CreateContextAndHandleEventThread(void* evt);
bool WINAPI CreateContextAndHandleEvent(Event* data);
DWORD WINAPI EventThread(LPVOID lpParam);
bool callEventFunction(JSContext* cx ,Event* evt);