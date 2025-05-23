//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

//#ifndef NPC_HGRUNT_H
#define NPC_HGRUNT_H
#ifdef _WIN32
#pragma once
#endif

#include "ai_basenpc.h"
#include "ai_basehumanoid.h"
#include "ai_behavior.h"
#include "ai_behavior_assault.h"
#include "ai_behavior_standoff.h"
#include "ai_behavior_follow.h"
#include "ai_behavior_functank.h"
#include "ai_behavior_rappel.h"
#include "ai_behavior_actbusy.h"
#include "ai_sentence.h"
#include "ai_baseactor.h"

// Used when only what HGRUNT to react to what the spotlight sees
#define SF_HGRUNT_NO_LOOK	(1 << 16)
#define SF_HGRUNT_NO_GRENADEDROP ( 1 << 17 )
#define SF_HGRUNT_NO_AR2DROP ( 1 << 18 )

//=========================================================
//	>> CNPC_HgruntBase
//=========================================================
class CNPC_HgruntBase : public CAI_BaseActor
{
	DECLARE_DATADESC();
	DEFINE_CUSTOM_AI;
	DECLARE_CLASS( CNPC_HgruntBase, CAI_BaseActor );

public:
	CNPC_HgruntBase();

	// Create components
	virtual bool	CreateComponents();

	bool			CanThrowGrenade( const Vector &vecTarget );
	bool			CheckCanThrowGrenade( const Vector &vecTarget );
	virtual	bool	CanGrenadeEnemy( bool bUseFreeKnowledge = true );
	virtual bool	CanAltFireEnemy( bool bUseFreeKnowledge );
	int				GetGrenadeConditions( float flDot, float flDist );
	int				RangeAttack2Conditions( float flDot, float flDist ); // For innate grenade attack
	int				MeleeAttack1Conditions( float flDot, float flDist ); // For kick/punch
	bool			FVisible( CBaseEntity *pEntity, int traceMask = MASK_BLOCKLOS, CBaseEntity **ppBlocker = NULL );
	virtual bool	IsCurTaskContinuousMove();

	virtual float	GetJumpGravity() const		{ return 1.8f; }

	virtual Vector  GetCrouchEyeOffset( void );

	void Event_Killed( const CTakeDamageInfo &info );
	int			OnTakeDamage_Alive(const CTakeDamageInfo &info);

	void SetActivity( Activity NewActivity );
	NPC_STATE		SelectIdealState ( void );

	// Input handlers.
	void InputLookOn( inputdata_t &inputdata );
	void InputLookOff( inputdata_t &inputdata );
	void InputStartPatrolling( inputdata_t &inputdata );
	void InputStopPatrolling( inputdata_t &inputdata );
	void InputAssault( inputdata_t &inputdata );
	void InputHitByBugbait( inputdata_t &inputdata );
	void InputThrowGrenadeAtTarget( inputdata_t &inputdata );

	bool			UpdateEnemyMemory( CBaseEntity *pEnemy, const Vector &position, CBaseEntity *pInformer = NULL );

	void			Spawn( void );
	void			Precache( void );
	void			Activate();

	Class_T			Classify( void );
	bool			IsElite() { return m_fIsElite; }
	void			DelayAltFireAttack( float flDelay );
	void			DelaySquadAltFireAttack( float flDelay );
	float			MaxYawSpeed( void );
	bool			ShouldMoveAndShoot();
	bool			OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval );
	void			HandleAnimEvent( animevent_t *pEvent );
	Vector			Weapon_ShootPosition( );

	Vector			EyeOffset( Activity nActivity );
	Vector			EyePosition( void );
	Vector			BodyTarget( const Vector &posSrc, bool bNoisy = true );
	Vector			GetAltFireTarget();

	void			StartTask( const Task_t *pTask );
	void			RunTask( const Task_t *pTask );
	void			PostNPCInit();
	void			GatherConditions();
	virtual void	PrescheduleThink();

	Activity		NPC_TranslateActivity( Activity eNewActivity );
	void			BuildScheduleTestBits( void );
	virtual int		SelectSchedule( void );
	virtual int		SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode );
	int				SelectScheduleAttack();

	bool			CreateBehaviors();

	bool			OnBeginMoveAndShoot();
	void			OnEndMoveAndShoot();

	// Combat
	WeaponProficiency_t CalcWeaponProficiency( CBaseCombatWeapon *pWeapon );
	bool			HasShotgun();
	bool			ActiveWeaponIsFullyLoaded();

	bool			HandleInteraction(int interactionType, void *data, CBaseCombatCharacter *sourceEnt);
	const char*		GetSquadSlotDebugName( int iSquadSlot );

	bool			IsUsingTacticalVariant( int variant );
	bool			IsUsingPathfindingVariant( int variant ) { return m_iPathfindingVariant == variant; }

	bool			IsRunningApproachEnemySchedule();

	// -------------
	// Sounds
	// -------------
	void			DeathSound( void );
	void			PainSound( void );
	void			IdleSound( void );
	void			AlertSound( void );
	void			LostEnemySound( void );
	void			FoundEnemySound( void );
	void			AnnounceAssault( void );
	void			AnnounceEnemyType( CBaseEntity *pEnemy );
	void			AnnounceEnemyKill( CBaseEntity *pEnemy );

	void			NotifyDeadFriend( CBaseEntity* pFriend );

	virtual float	HearingSensitivity( void ) { return 1.0; };
	int				GetSoundInterests( void );
	virtual bool	QueryHearSound( CSound *pSound );

	// Speaking
	void			SpeakSentence( int sentType );

	virtual int		TranslateSchedule( int scheduleType );
	void			OnStartSchedule( int scheduleType );

	virtual bool	ShouldPickADeathPose( void );

protected:
	void			SetKickDamage( int nDamage ) { m_nKickDamage = nDamage; }
	CAI_Sentence< CNPC_HgruntBase > *GetSentences() { return &m_Sentences; }

private:
	//=========================================================
	// HGRUNT S schedules
	//=========================================================
	enum
	{
		SCHED_HGRUNT_SUPPRESS = BaseClass::NEXT_SCHEDULE,
		SCHED_HGRUNT_COMBAT_FAIL,
		SCHED_HGRUNT_VICTORY_DANCE,
		SCHED_HGRUNT_COMBAT_FACE,
		SCHED_HGRUNT_HIDE_AND_RELOAD,
		SCHED_HGRUNT_SIGNAL_SUPPRESS,
		SCHED_HGRUNT_ENTER_OVERWATCH,
		SCHED_HGRUNT_OVERWATCH,
		SCHED_HGRUNT_ASSAULT,
		SCHED_HGRUNT_ESTABLISH_LINE_OF_FIRE,
		SCHED_HGRUNT_PRESS_ATTACK,
		SCHED_HGRUNT_WAIT_IN_COVER,
		SCHED_HGRUNT_RANGE_ATTACK1,
		SCHED_HGRUNT_RANGE_ATTACK2,
		SCHED_HGRUNT_TAKE_COVER1,
		SCHED_HGRUNT_TAKE_COVER_FROM_BEST_SOUND,
		SCHED_HGRUNT_RUN_AWAY_FROM_BEST_SOUND,
		SCHED_HGRUNT_GRENADE_COVER1,
		SCHED_HGRUNT_TOSS_GRENADE_COVER1,
		SCHED_HGRUNT_TAKECOVER_FAILED,
		SCHED_HGRUNT_GRENADE_AND_RELOAD,
		SCHED_HGRUNT_PATROL,
		SCHED_HGRUNT_BUGBAIT_DISTRACTION,
		SCHED_HGRUNT_CHARGE_TURRET,
		SCHED_HGRUNT_DROP_GRENADE,
		SCHED_HGRUNT_CHARGE_PLAYER,
		SCHED_HGRUNT_PATROL_ENEMY,
		SCHED_HGRUNT_BURNING_STAND,
		SCHED_HGRUNT_AR2_ALTFIRE,
		SCHED_HGRUNT_FORCED_GRENADE_THROW,
		SCHED_HGRUNT_MOVE_TO_FORCED_GREN_LOS,
		SCHED_HGRUNT_FACE_IDEAL_YAW,
		SCHED_HGRUNT_MOVE_TO_MELEE,
		NEXT_SCHEDULE,
	};

	//=========================================================
	// HGRUNT Tasks
	//=========================================================
	enum 
	{
		TASK_HGRUNT_FACE_TOSS_DIR = BaseClass::NEXT_TASK,
		TASK_HGRUNT_IGNORE_ATTACKS,
		TASK_HGRUNT_SIGNAL_BEST_SOUND,
		TASK_HGRUNT_DEFER_SQUAD_GRENADES,
		TASK_HGRUNT_CHASE_ENEMY_CONTINUOUSLY,
		TASK_HGRUNT_DIE_INSTANTLY,
		TASK_HGRUNT_PLAY_SEQUENCE_FACE_ALTFIRE_TARGET,
		TASK_HGRUNT_GET_PATH_TO_FORCED_GREN_LOS,
		TASK_HGRUNT_SET_STANDING,
		NEXT_TASK
	};

	//=========================================================
	// HGRUNT Conditions
	//=========================================================
	enum HGRUNT_Conds
	{
		COND_HGRUNT_NO_FIRE = BaseClass::NEXT_CONDITION,
		COND_HGRUNT_DEAD_FRIEND,
		COND_HGRUNT_SHOULD_PATROL,
		COND_HGRUNT_HIT_BY_BUGBAIT,
		COND_HGRUNT_DROP_GRENADE,
		COND_HGRUNT_ON_FIRE,
		COND_HGRUNT_ATTACK_SLOT_AVAILABLE,
		NEXT_CONDITION
	};

private:
	// Select the combat schedule
	int SelectCombatSchedule();

	// Should we charge the player?
	bool ShouldChargePlayer();

	// Chase the enemy, updating the target position as the player moves
	void StartTaskChaseEnemyContinuously( const Task_t *pTask );
	void RunTaskChaseEnemyContinuously( const Task_t *pTask );

	class CHGRUNTStandoffBehavior : public CAI_ComponentWithOuter<CNPC_HgruntBase, CAI_StandoffBehavior>
	{
		typedef CAI_ComponentWithOuter<CNPC_HgruntBase, CAI_StandoffBehavior> BaseClass;

		virtual int SelectScheduleAttack()
		{
			int result = GetOuter()->SelectScheduleAttack();
			if ( result == SCHED_NONE )
				result = BaseClass::SelectScheduleAttack();
			return result;
		}
	};

	// Rappel
	virtual bool IsWaitingToRappel( void ) { return m_RappelBehavior.IsWaitingToRappel(); }
	void BeginRappel() { m_RappelBehavior.BeginRappel(); }

private:
	int				m_nKickDamage;
	Vector			m_vecTossVelocity;
	EHANDLE			m_hForcedGrenadeTarget;
	bool			m_bShouldPatrol;
	bool			m_bFirstEncounter;// only put on the handsign show in the squad's first encounter.

	// Time Variables
	float			m_flNextPainSoundTime;
	float			m_flNextAlertSoundTime;
	float			m_flNextGrenadeCheck;	
	float			m_flNextLostSoundTime;
	float			m_flAlertPatrolTime;		// When to stop doing alert patrol
	float			m_flNextAltFireTime;		// Elites only. Next time to begin considering alt-fire attack.

	int				m_nShots;
	float			m_flShotDelay;
	float			m_flStopMoveShootTime;

	CAI_Sentence< CNPC_HgruntBase > m_Sentences;

	int			m_iNumGrenades;
	CAI_AssaultBehavior			m_AssaultBehavior;
	CHGRUNTStandoffBehavior	m_StandoffBehavior;
	CAI_FollowBehavior			m_FollowBehavior;
	CAI_FuncTankBehavior		m_FuncTankBehavior;
	CAI_RappelBehavior			m_RappelBehavior;
	CAI_ActBusyBehavior			m_ActBusyBehavior;

public:
	int				m_iLastAnimEventHandled;
	bool			m_fIsElite;
	Vector			m_vecAltFireTarget;

	int				m_iTacticalVariant;
	int				m_iPathfindingVariant;
};


//#endif
