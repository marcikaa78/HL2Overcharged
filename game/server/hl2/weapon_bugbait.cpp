//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "basehlcombatweapon.h"
#include "engine/IEngineSound.h"
#include "npcevent.h"
#include "in_buttons.h"
#include "antlion_maker.h"
#include "grenade_bugbait.h"
#include "gamestats.h"
#include "in_buttons.h"
#include "player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//
// Bug Bait Weapon
//

class CWeaponBugBait : public CBaseHLCombatWeapon
{
	DECLARE_CLASS( CWeaponBugBait, CBaseHLCombatWeapon );
public:

	DECLARE_SERVERCLASS();

	CWeaponBugBait( void );

	void	Spawn( void );
	void	FallInit( void );

	int		CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }
	
	void	Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );

	void	Drop( const Vector &vecVelocity );
	void	BugbaitStickyTouch( CBaseEntity *pOther );
	void	OnPickedUp( CBaseCombatCharacter *pNewOwner );
	bool	Deploy( void );
	bool	Holster( CBaseCombatWeapon *pSwitchingTo );

	void	ItemPostFrame( void );
	void	Precache( void );
	void	PrimaryAttack( void );
	void	SecondaryAttack( void );
	void	ThrowGrenade( CBasePlayer *pPlayer );
	
	bool	HasAnyAmmo( void ) { return true; }
	
	bool	Reload( void );

	bool CanInspect(CBasePlayer *pOwner)
	{
		return true;
	}

	void	SetSporeEmitterState( bool state = true );

	bool	ShouldDisplayHUDHint() { return true; }

	DECLARE_DATADESC();
	DECLARE_ACTTABLE();			//added

protected:

	bool		m_bDrawBackFinished;
	bool		m_bRedraw;
	bool		m_bEmitSpores;
	bool		m_bDoEffects;
	float		m_flNextPuff;
	EHANDLE		m_hSporeTrail;
};

IMPLEMENT_SERVERCLASS_ST(CWeaponBugBait, DT_WeaponBugBait)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_bugbait, CWeaponBugBait );
#ifndef HL2MP
PRECACHE_WEAPON_REGISTER( weapon_bugbait );
#endif

/// overcharged: player anims
acttable_t	CWeaponBugBait::m_acttable[] =
{
	{ ACT_HL2MP_IDLE, ACT_HL2MP_IDLE_GRENADE, false },
	{ ACT_HL2MP_RUN, ACT_HL2MP_RUN_GRENADE, false },
	{ ACT_HL2MP_IDLE_CROUCH, ACT_HL2MP_IDLE_CROUCH_GRENADE, false },
	{ ACT_HL2MP_WALK_CROUCH, ACT_HL2MP_WALK_CROUCH_GRENADE, false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK, ACT_HL2MP_GESTURE_RANGE_ATTACK_GRENADE, false },
	{ ACT_HL2MP_GESTURE_RELOAD, ACT_HL2MP_GESTURE_RELOAD_GRENADE, false },
	{ ACT_HL2MP_JUMP, ACT_HL2MP_JUMP_SLAM, false },

	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_THROW, true },

	{ ACT_IDLE_RELAXED, ACT_IDLE, false },
	{ ACT_RUN_RELAXED, ACT_RUN, false },
	{ ACT_WALK_RELAXED, ACT_WALK, false },
};
IMPLEMENT_ACTTABLE(CWeaponBugBait);

BEGIN_DATADESC( CWeaponBugBait )

	DEFINE_FIELD( m_hSporeTrail,		FIELD_EHANDLE ),
	DEFINE_FIELD( m_bRedraw,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bEmitSpores,		FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bDrawBackFinished,	FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bDoEffects,			FIELD_BOOLEAN),
	DEFINE_FIELD( m_flNextPuff,			FIELD_TIME ),
	DEFINE_FUNCTION( BugbaitStickyTouch ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CWeaponBugBait::CWeaponBugBait( void )
{
	m_bDrawBackFinished	= false;
	m_bRedraw			= false;
	m_hSporeTrail		= NULL;
	m_bDoEffects		= false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponBugBait::Spawn( void )
{
	BaseClass::Spawn();

	// Increase the bugbait's pickup volume. It spawns inside the antlion guard's body,
	// and playtesters seem to be wary about moving into the body.
	SetSize( Vector( -4, -4, -4), Vector(4, 4, 4) );
	CollisionProp()->UseTriggerBounds( true, 100 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponBugBait::FallInit( void )
{
	// Bugbait shouldn't be physics, because it musn't roll/move away from it's spawnpoint.
	// The game will break if the player can't pick it up, so it must stay still.
	SetModel( GetWorldModel() );

	VPhysicsDestroyObject();
	SetMoveType( MOVETYPE_FLYGRAVITY );
	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_TRIGGER );

	SetPickupTouch();
	
	SetThink( &CBaseCombatWeapon::FallThink );

	SetNextThink( gpGlobals->curtime + 0.1f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponBugBait::Precache( void )
{
	BaseClass::Precache();

	UTIL_PrecacheOther( "npc_grenade_bugbait" );

	PrecacheScriptSound( "Weapon_Bugbait.Splat" );

	PrecacheParticleSystem("bugbait_puff");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponBugBait::Drop( const Vector &vecVelocity )
{
	BaseClass::Drop( vecVelocity );

	// On touch, stick & stop moving. Increase our thinktime a bit so we don't stomp the touch for a bit
	SetNextThink( gpGlobals->curtime + 3.0 );
	SetTouch( &CWeaponBugBait::BugbaitStickyTouch );

	m_hSporeTrail = SporeExplosion::CreateSporeExplosion();
	if ( m_hSporeTrail )
	{
		SporeExplosion *pSporeExplosion = (SporeExplosion *)m_hSporeTrail.Get();

		QAngle	angles;
		VectorAngles( Vector(0,0,1), angles );

		pSporeExplosion->SetAbsAngles( angles );
		pSporeExplosion->SetAbsOrigin( GetAbsOrigin() );
		pSporeExplosion->SetParent( this );

		pSporeExplosion->m_flSpawnRate			= 16.0f;
		pSporeExplosion->m_flParticleLifetime	= 0.5f;
		pSporeExplosion->SetRenderColor( 0.0f, 0.5f, 0.25f, 0.15f );

		pSporeExplosion->m_flStartSize			= 32;
		pSporeExplosion->m_flEndSize			= 48;
		pSporeExplosion->m_flSpawnRadius		= 4;

		pSporeExplosion->SetLifetime( 9999 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Stick to the world when we touch it
//-----------------------------------------------------------------------------
void CWeaponBugBait::BugbaitStickyTouch( CBaseEntity *pOther )
{
	if ( !pOther->IsWorld() )
		return;

	// Stop moving, wait for pickup
	SetMoveType( MOVETYPE_NONE );
	SetThink( NULL );
	SetPickupTouch();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPicker - 
//-----------------------------------------------------------------------------
void CWeaponBugBait::OnPickedUp( CBaseCombatCharacter *pNewOwner )
{
	BaseClass::OnPickedUp( pNewOwner );

	if ( m_hSporeTrail )
	{
		UTIL_Remove( m_hSporeTrail );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponBugBait::PrimaryAttack( void )
{
	if ( m_bRedraw )
		return;

	CBaseCombatCharacter *pOwner  = GetOwner();
	
	if ( pOwner == NULL )
		return;

	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	
	if ( pPlayer == NULL )
		return;

	if (IsNearWall() || GetOwnerIsRunning())
	{
		return;
	}

	SendWeaponAnim(GetWpnData().animData[m_bFireMode].ThrowPullUp);
	
	m_flTimeWeaponIdle = gpGlobals->curtime + GetViewModelSequenceDuration();//FLT_MAX;
	m_flNextPrimaryAttack = gpGlobals->curtime + GetViewModelSequenceDuration();//FLT_MAX;

	m_iPrimaryAttacks++;
	gamestats->Event_WeaponFired( pPlayer, true, GetClassname() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponBugBait::SecondaryAttack( void )
{
	// Squeeze!
	CPASAttenuationFilter filter( this );

	EmitSound( filter, entindex(), "Weapon_Bugbait.Splat" );

	if ( CGrenadeBugBait::ActivateBugbaitTargets( GetOwner(), GetAbsOrigin(), true ) == false )
	{
		g_AntlionMakerManager.BroadcastFollowGoal( GetOwner() );
	}

	SendWeaponAnim(GetWpnData().animData[m_bFireMode].ThrowSecondary);
	m_flNextSecondaryAttack = gpGlobals->curtime + SequenceDuration();

	m_bDoEffects = true;
	m_flNextPuff = gpGlobals->curtime + 0.4f;

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner )
	{
		m_iSecondaryAttacks++;
		gamestats->Event_WeaponFired( pOwner, false, GetClassname() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CWeaponBugBait::ThrowGrenade( CBasePlayer *pPlayer )
{
	Vector	vForward, vRight, vUp, vThrowPos, vThrowVel;
	
	pPlayer->EyeVectors( &vForward, &vRight, &vUp );

	vThrowPos = pPlayer->EyePosition();

	vThrowPos += vForward * 18.0f;
	vThrowPos += vRight * 12.0f;

	pPlayer->GetVelocity( &vThrowVel, NULL );
	vThrowVel += vForward * 1000;

	CGrenadeBugBait *pGrenade = BugBaitGrenade_Create( vThrowPos, vec3_angle, vThrowVel, QAngle(600,random->RandomInt(-1200,1200),0), pPlayer );

	if ( pGrenade != NULL )
	{
		// If the shot is clear to the player, give the missile a grace period
		trace_t	tr;
		UTIL_TraceLine( pPlayer->EyePosition(), pPlayer->EyePosition() + ( vForward * 128 ), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
		
		if ( tr.fraction == 1.0 )
		{
			pGrenade->SetGracePeriod( 0.1f );
		}
	}

	m_bRedraw = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEvent - 
//			*pOperator - 
//-----------------------------------------------------------------------------
void CWeaponBugBait::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	switch( pEvent->event )
	{
		case EVENT_WEAPON_SEQUENCE_FINISHED:
			m_bDrawBackFinished = true;
			break;

		case EVENT_WEAPON_THROW:
			ThrowGrenade( pOwner );
			break;

		default:
			BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
			break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponBugBait::Reload( void )
{
	if ( ( m_bRedraw ) && ( m_flNextPrimaryAttack <= gpGlobals->curtime ) )
	{
		//Redraw the weapon
		SendWeaponAnim( ACT_VM_DRAW );

		//Update our times
		m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();

		//Mark this as done
		m_bRedraw = false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponBugBait::ItemPostFrame( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return;

	//CheckAdmireAnimations(pOwner);

	if (m_flNextPuff < gpGlobals->curtime && m_bDoEffects)
	{
		/*Vector dir = pOwner->EyeDirection3D();
		
		dir *= 3.f;

		Vector direction = dir;*/

		CBaseViewModel *pVM = pOwner->GetViewModel();

		if (pVM)
		{
			DispatchParticleEffect("bugbait_puff", PATTACH_POINT_FOLLOW, pVM, pVM->LookupAttachment("0"), false);

			/*Vector Spore;
			int iAttachment = pVM->LookupAttachment("0"); pVM->GetAttachment(iAttachment, Spore);

			Spore += dir;

			for (int i = 0; i < 1; i++)
			{
				//Do effect for the hit
				SporeExplosion *pSporeExplosion = SporeExplosion::CreateSporeExplosion();

				if (pSporeExplosion)
				{
					VectorNormalize(direction);

					QAngle	angles;
					VectorAngles(direction, angles);

					pSporeExplosion->SetLocalAngles(angles);
					pSporeExplosion->SetLocalOrigin(Spore);
					pSporeExplosion->m_flSpawnRate = 8.0f;
					pSporeExplosion->m_flParticleLifetime = 2.0f;
					pSporeExplosion->SetRenderColor(0.0f, 0.5f, 0.25f, 0.15f);

					pSporeExplosion->m_flStartSize = 32.0f;
					pSporeExplosion->m_flEndSize = 64.0f;
					pSporeExplosion->m_flSpawnRadius = 32.0f;

					pSporeExplosion->SetLifetime(bugbait_distract_time.GetFloat());
				}
			}*/
		}

		m_bDoEffects = false;
	}

	// See if we're cocked and ready to throw
	if ( m_bDrawBackFinished )
	{
		if ( ( pOwner->m_nButtons & IN_ATTACK ) == false )
		{
			SendWeaponAnim(GetWpnData().animData[m_bFireMode].ThrowPrimary);
			m_flNextPrimaryAttack  = gpGlobals->curtime + SequenceDuration();
			m_bDrawBackFinished = false;
		}
	}
	else
	{
		//See if we're attacking
		if ( ( pOwner->m_nButtons & IN_ATTACK ) && ( m_flNextPrimaryAttack < gpGlobals->curtime ) )
		{
			PrimaryAttack();
		}
		else if ( ( pOwner->m_nButtons & IN_ATTACK2 ) && ( m_flNextSecondaryAttack < gpGlobals->curtime ) )
		{
			SecondaryAttack();
		}
	}

	if ( m_bRedraw )
	{
		if ( IsViewModelSequenceFinished() )
		{
			Reload();
		}
	}

	if (!(pOwner->m_nButtons & IN_ATTACK) && !(pOwner->m_nButtons & IN_ATTACK2) && gpGlobals->curtime > m_flTimeWeaponIdle)
		WeaponIdle();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponBugBait::Deploy( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner == NULL )
		return false;

	/*
	if ( m_hSporeTrail == NULL )
	{
		m_hSporeTrail = SporeTrail::CreateSporeTrail();
		
		m_hSporeTrail->m_bEmit				= true;
		m_hSporeTrail->m_flSpawnRate		= 100.0f;
		m_hSporeTrail->m_flParticleLifetime	= 2.0f;
		m_hSporeTrail->m_flStartSize		= 1.0f;
		m_hSporeTrail->m_flEndSize			= 4.0f;
		m_hSporeTrail->m_flSpawnRadius		= 8.0f;

		m_hSporeTrail->m_vecEndColor		= Vector( 0, 0, 0 );

		CBaseViewModel *vm = pOwner->GetViewModel();
		
		if ( vm != NULL )
		{
			m_hSporeTrail->FollowEntity( vm );
		}
	}
	*/

	m_bRedraw = false;
	m_bDrawBackFinished = false;

	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponBugBait::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	m_bRedraw = false;
	m_bDrawBackFinished = false;

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : true - 
//-----------------------------------------------------------------------------
void CWeaponBugBait::SetSporeEmitterState( bool state )
{
	m_bEmitSpores = state;
}
