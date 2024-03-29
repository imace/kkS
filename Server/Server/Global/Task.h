/************************************************************************/
/*                                                                      */
/************************************************************************/

#ifndef __SERVICE_H__
#define __SERVICE_H__
#include "EventMgr.h"
//////////////////////////////////////////////////////////////////////////
LOG_DECL(ServiceMgrLog);
//////////////////////////////////////////////////////////////////////////

struct InvokerStatus
{
public:
	enum 
	{
		IDLE,
		READY,
		SCHUDULE,
		EXECUTE
	};
};

struct  InvokerType
{
public:
	enum
	{
		ACTIVE,
		PASSIVE,
	};
};

class Invoker
{	
public:
	Invoker(uint32_t interval, int32_t type = InvokerType::ACTIVE, int32_t initState = InvokerStatus::READY);
	virtual ~Invoker();
public:
	virtual void	UpdateTimeInfo();
	void			UpdateInvokeTimeLeft(int32_t elapse);
	void			Invoke();
	virtual void	Do() = 0;
public:
	bool			CanExcuteNow( ) const				{ return m_InvokeTimeLeft <= 0; }
	void			UpdateSchuduleTime(int32_t elapse)	{ m_SchuduleTime += elapse; }
	void			ResetIdleTime()						{ m_IdleTime = 0; }
	void			UpdateIdleTime(int32_t elapse)		{ m_IdleTime += elapse; }
	int32_t			GetIdleTime( ) const				{ return m_IdleTime; }
	int32_t			GetSchuduleTime( ) const			{ return m_SchuduleTime; }
	void			UpdateExcuteTime(int32_t elapse)	{ m_ExcuteTime += elapse; }
	int32_t			GetExcuteTime( ) const				{ return m_ExcuteTime; }
	uint32_t		GetInterval() const					{ return m_Interval; }
	const TimeInfo& GetTimeInfo() const					{ return m_TimeInfo; }
	bool			IsState(uint32_t state) const		{ return (m_State == state); }
	void			SetState(uint32_t state)			{ m_State = state; }
	uint32_t		GetState( ) const					{ return m_State; }
	uint32_t		GetType( ) const					{ return m_Type; }
protected:
	TimeInfo		m_TimeInfo;
	uint32_t		m_Interval;
	int32_t			m_InvokeTimeLeft;
	int32_t			m_SchuduleTime;
	int32_t			m_ExcuteTime;
	int32_t			m_IdleTime;
	uint32_t		m_State;
	uint32_t		m_Type;
};

typedef boost::shared_ptr<Invoker> InvokerPtr;

//////////////////////////////////////////////////////////////////////////
template<class InvokerImpl, int32_t InterVal>
class MakeInvoker : public Invoker
{
public:
	MakeInvoker(InvokerImpl& rInvokerImpl):Invoker(InterVal), m_rInvokerImpl(rInvokerImpl){}
	virtual ~MakeInvoker() {}
public:
	virtual void Do()
	{
		UpdateTimeInfo( );
		m_rInvokerImpl.Tick(m_TimeInfo);
	}

private:
	InvokerImpl& m_rInvokerImpl;
};


//////////////////////////////////////////////////////////////////////////
struct ServiceStatus
{
public:
	enum State
	{
		STOP = 0,

		START,
		START_EXC,
		START_OK,

		LOAD,
		LOAD_EXC,
		LOAD_OK,

		EXECUTE,

		SHUTDOWN,
		SHUTDOWN_EXC,
		SHUTDOWN_OK,

		FINALSAVE,
		FINALSAVE_EXC,
		FINALSAVE_OK,
	};
};


class Service : public EventMgr
{
public:
	virtual void		Tick(const TimeInfo& rTimeInfo);
private:
	virtual void		Tick_State();
protected:
	virtual	void		Start();
	virtual	void		Load();
	virtual void		Shutdown();
	virtual void		FinalSave();
public:
	virtual void		OnStartOk();
	virtual void		OnLoadOk();
	virtual	void		OnShutdownOk();
	virtual	void		OnFinalSaveOk();
public:
	void				SetState(uint32_t state){ m_State = state; }
	uint32_t			GetState( ) const		{ return m_State; }
	bool				IsState(uint32_t state) const { return (m_State == state); }
private:
	uint32_t m_State;
public:
	Service();
	virtual ~Service();
public:
	virtual bool		Init() = 0;
	virtual int32_t		GetServiceID() = 0;
public:
	InvokerPtr			FetchInvoker( );
	void				AddInvoker(InvokerPtr taskPtr);
private:
	TSList<InvokerPtr>	m_InvokerPtrList;
};

typedef boost::shared_ptr<Service> ServicePtr;

//////////////////////////////////////////////////////////////////////////

typedef boost::threadpool::fifo_pool  ThreadPool;
typedef boost::shared_ptr<ThreadPool> ThreadPoolPtr;
class ServiceMgr
{
	const  static int32_t SERVICEMANAGER_NAME_LEN = 32;
	friend class MapCreator;
public:
	ServiceMgr();
	~ServiceMgr();
public:
	bool					Init(int32_t maxTask, int32_t maxThread);
	bool					Register(ServicePtr taskPtr);
	void					Excute();
	void					Exit();
public:
	void					SetName(const CHAR* Name) { strncpy(m_Name, Name, SERVICEMANAGER_NAME_LEN);  }
	const CHAR*				GetName() const { return m_Name; }
private:
	void					SetAllInvokerState_MainThread(int32_t state);
	void					SetAllServiceState(int32_t state);
	void					ExcuteState(int32_t setState, int32_t checkState);
	void					ExcuteAllServiceInit();
	void					ExcuteAllServiceStart();
	void					ExcuteAllServiceLoad();
	void					ExcuteAllService();
	void					ExcuteAllServiceShutdown();
	void					ExcuteAllServiceFinalSave();
private:
	void					Tick(int32_t elapse);
	void					Tick_Service(int32_t elapse);
	void					Tick_AllInvoker(int32_t elapse);
	void					Tick_Logic(int32_t elapse);
private:
	void					Wait(int32_t sec);
private:
	bool					IsAllInvokerInState(int32_t state);
	bool					IsAllTaskInState(int32_t state);
	bool					IsShouldShutdown();
private:
	CHAR					m_Name[SERVICEMANAGER_NAME_LEN];
	TimeInfo				m_TimeInfo;
	ThreadPoolPtr			m_ThreadPoolPtr;
	TVector<ServicePtr>		m_ServicePtrVec;
	TList<InvokerPtr>		m_InvokerPtrList;				
};

#endif
