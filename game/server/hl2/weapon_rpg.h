//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef WEAPON_RPG_H
#define WEAPON_RPG_H

#ifdef _WIN32
#pragma once
#endif

#include "basehlcombatweapon.h"
#include "Sprite.h"
#include "npcevent.h"
#include "beam_shared.h"
#include "saverestore_utlvector.h"

class CWeaponRPG;
class CLaserDot;
class RocketTrail;
class CParticleSystem;
//###########################################################################
//	>> CMissile		(missile launcher class is below this one!)
//###########################################################################
class CMissile : public CBaseCombatCharacter
{
	DECLARE_CLASS( CMissile, CBaseCombatCharacter );

public:
	static const int EXPLOSION_RADIUS = 200;

	CMissile();
	~CMissile();

#ifdef HL1_DLL
	Class_T Classify( void ) { return CLASS_NONE; }
#else
	Class_T Classify( void ) { return CLASS_MISSILE; }
#endif
	
	void	Spawn( void );
	void	Precache( void );
	void	MissileTouch( CBaseEntity *pOther );
	void	Explode( void );
	void	ShotDown( void );
	void	AccelerateThink( void );
	void	AugerThink( void );
	void	IgniteThink( void );
	void	SeekThink( void );
	void	DumbFire( void );
	void	SetGracePeriod( float flGracePeriod );

	int		OnTakeDamage_Alive( const CTakeDamageInfo &info );
	void	Event_Killed( const CTakeDamageInfo &info );
	
	virtual float	GetDamage() { return m_flDamage; }
	virtual void	SetDamage(float flDamage) { m_flDamage = flDamage; }

	unsigned int PhysicsSolidMaskForEntity( void ) const;

	CHandle<CWeaponRPG>		m_hOwner;

	static CMissile *Create( const Vector &vecOrigin, const QAngle &vecAngles, edict_t *pentOwner );

	void CreateDangerSounds( bool bState ){ m_bCreateDangerSounds = bState; }

	static void AddCustomDetonator( CBaseEntity *pEntity, float radius, float height = -1 );
	static void RemoveCustomDetonator( CBaseEntity *pEntity );

protected:
	virtual void DoExplosion();	
	virtual void ComputeActualDotPosition( CLaserDot *pLaserDot, Vector *pActualDotPosition, float *pHomingSpeed );
	virtual int AugerHealth() { return m_iMaxHealth - 20; }

	// Creates the smoke trail
	void CreateSmokeTrail( void );

	// Gets the shooting position 
	void GetShootPosition( CLaserDot *pLaserDot, Vector *pShootPosition );

	CHandle<RocketTrail>	m_hRocketTrail;
	//CHandle< CParticleSystem >	m_hRocketTrail;
	float					m_flAugerTime;		// Amount of time to auger before blowing up anyway
	float					m_flMarkDeadTime;
	float					m_flDamage;

	struct CustomDetonator_t
	{
		EHANDLE hEntity;
		float radiusSq;
		float halfHeight;
	};

	static CUtlVector<CustomDetonator_t> gm_CustomDetonators;

private:
	float					m_flGracePeriodEndsAt;
	bool					m_bCreateDangerSounds;
	DECLARE_DATADESC();
};


//-----------------------------------------------------------------------------
// Laser dot control
//-----------------------------------------------------------------------------
CBaseEntity *CreateLaserDot( const Vector &origin, CBaseEntity *pOwner, bool bVisibleDot );
void SetLaserDotTarget( CBaseEntity *pLaserDot, CBaseEntity *pTarget );
void EnableLaserDot( CBaseEntity *pLaserDot, bool bEnable );


//-----------------------------------------------------------------------------
// Specialized mizzizzile
//-----------------------------------------------------------------------------
class CAPCMissile : public CMissile
{
	DECLARE_CLASS( CMissile, CMissile );
	DECLARE_DATADESC();

public:
	static CAPCMissile *Create( const Vector &vecOrigin, const QAngle &vecAngles, const Vector &vecVelocity, CBaseEntity *pOwner );

	CAPCMissile();
	~CAPCMissile();
	void	IgniteDelay( void );
	void	AugerDelay( float flDelayTime );
	void	ExplodeDelay( float flDelayTime );
	void	DisableGuiding();
#if defined( HL2_DLL )
	virtual Class_T Classify ( void ) { return CLASS_COMBINE; }
#endif

	void	AimAtSpecificTarget( CBaseEntity *pTarget );
	void	SetGuidanceHint( const char *pHintName );

	void	APCSeekThink( void );

	CAPCMissile			*m_pNext;

protected:
	virtual void DoExplosion();	
	virtual void ComputeActualDotPosition( CLaserDot *pLaserDot, Vector *pActualDotPosition, float *pHomingSpeed );
	virtual int AugerHealth();

private:
	void Init();
	void ComputeLeadingPosition( const Vector &vecShootPosition, CBaseEntity *pTarget, Vector *pLeadPosition );
	void BeginSeekThink();
	void AugerStartThink();
	void ExplodeThink();
	void APCMissileTouch( CBaseEntity *pOther );

	float	m_flReachedTargetTime;
	float	m_flIgnitionTime;
	bool	m_bGuidingDisabled;
	float   m_flLastHomingSpeed;
	EHANDLE m_hSpecificTarget;
	string_t m_strHint;
};


//-----------------------------------------------------------------------------
// Finds apc missiles in cone
//-----------------------------------------------------------------------------
CAPCMissile *FindAPCMissileInCone( const Vector &vecOrigin, const Vector &vecDirection, float flAngle );


//-----------------------------------------------------------------------------
// RPG
//-----------------------------------------------------------------------------
class CWeaponRPG : public CBaseHLCombatWeapon
{
	DECLARE_CLASS( CWeaponRPG, CBaseHLCombatWeapon );
public:

	CWeaponRPG();
	~CWeaponRPG();

	DECLARE_SERVERCLASS();

	CBaseEntity		*GetMissile(void) { return m_hMissile; }
	const Vector	&GetNPCLaserPosition(void);
	virtual bool	IsGuiding(void);
	virtual void	NotifyRocketDied(void);
	virtual void	UpdateNPCLaserPosition(const Vector &vecTarget);
	virtual void	SetNPCLaserPosition(const Vector &vecTarget);
	virtual void	StartGuiding(void);
	virtual void	StopGuiding(void);
	virtual void	WeaponIdle(void);

protected:

	virtual void	Precache( void );

	virtual void	PrimaryAttack(void);
	virtual void	SecondaryAttack(void);

	virtual void	ItemPostFrame(void);

	virtual void	Activate(void);
	virtual void	DecrementAmmo(CBaseCombatCharacter *pOwner);

	virtual bool	Deploy(void);
	virtual bool	Holster(CBaseCombatWeapon *pSwitchingTo = NULL);
	virtual bool	Reload(void);
	virtual bool	WeaponShouldBeLowered(void);
	virtual bool	Lower(void);

	virtual void Drop( const Vector &vecVelocity );

	virtual float	GetMinRestTime() { return 4.0; }
	virtual float	GetMaxRestTime() { return 4.0; }

	virtual bool	WeaponLOSCondition(const Vector &ownerPos, const Vector &targetPos, bool bSetConditions);
	virtual int		WeaponRangeAttack1Condition(float flDot, float flDist);

	virtual void	Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator);

	virtual void	ToggleGuiding(void);

	virtual bool	HasAnyAmmo(void);

	virtual void	SuppressGuiding(bool state = true);

	virtual void	CreateLaserPointer(void);
	virtual void	UpdateLaserPosition(Vector vecMuzzlePos = vec3_origin, Vector vecEndPos = vec3_origin);
	virtual void	UpdateNpcLaserPosition(Vector vecMuzzlePos = vec3_origin, Vector vecEndPos = vec3_origin);
	virtual Vector	GetLaserPosition(void);
	//void	StartLaserEffects( void );
	//void	StopLaserEffects( void );
	virtual void	UpdateLaserEffects(void);

	// NPC RPG users cheat and directly set the laser pointer's origin

	virtual int		CapabilitiesGet(void) { return bits_CAP_WEAPON_RANGE_ATTACK1; }

	virtual const Vector& GetBulletSpread( void )
	{
		static Vector cone = VECTOR_CONE_3DEGREES;
		return cone;
	}

	bool fired;
	bool secondAttack;

	DECLARE_ACTTABLE();
	DECLARE_DATADESC();

	bool				m_bGuiding;
//	float   MuzzleFlashTime;//Счётчик для вспышки
	bool				m_bInitialStateUpdate;
	bool				m_bHideGuiding;		//User to override the player's wish to guide under certain circumstances
	Vector				m_vecNPCLaserDot;
	CHandle<CLaserDot>	m_hLaserDot;
	CHandle<CMissile>	m_hMissile;
	CHandle<CSprite>	m_hLaserMuzzleSprite;
	CHandle<CBeam>		m_hLaserBeam;
private:
	CUtlVector<CMissile*> allMissiles;
};

#endif // WEAPON_RPG_H
