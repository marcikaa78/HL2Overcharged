//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		A gib is a chunk of a body, or a piece of wood/metal/rocks/etc.
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//===========================================================================//

#include "cbase.h"
#include "gib.h"
#include "soundent.h"
#include "func_break.h"		// For materials
#include "player.h"
#include "vstdlib/random.h"
#include "ai_utils.h"
#include "EntityFlame.h"

#include "ragdoll_shared.h"
#include "baseanimating.h"
#include "physics_prop_ragdoll.h"
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern Vector			g_vecAttackDir;		// In globals.cpp

BEGIN_DATADESC( CGib )

	// gibs are not saved/restored
//	DEFINE_FIELD( m_bloodColor, FIELD_INTEGER ),
//	DEFINE_FIELD( m_hSprite, FIELD_EHANDLE ),
//	DEFINE_FIELD( m_cBloodDecals, FIELD_INTEGER ),
//	DEFINE_FIELD( m_material, FIELD_INTEGER ),
//	DEFINE_FIELD( m_lifeTime, FIELD_TIME ),
//	DEFINE_FIELD( m_pSprite, CSprite ),
//	DEFINE_FIELD( m_hFlame, FIELD_EHANDLE ),

//	DEFINE_FIELD( m_hPhysicsAttacker, FIELD_EHANDLE ),
//	DEFINE_FIELD( m_flLastPhysicsInfluenceTime, FIELD_TIME ),

//  DEFINE_FIELD( m_bForceRemove, FIELD_BOOLEAN ),

	// Function pointers
	DEFINE_ENTITYFUNC( BounceGibTouch ),
	DEFINE_ENTITYFUNC( StickyGibTouch ),
	DEFINE_THINKFUNC( WaitTillLand ),
	DEFINE_THINKFUNC( DieThink ),

END_DATADESC()


// HACKHACK -- The gib velocity equations don't work
void CGib::LimitVelocity( void )
{
	Vector vecNewVelocity = GetAbsVelocity();
	float length = VectorNormalize( vecNewVelocity );

	// ceiling at 1500.  The gib velocity equation is not bounded properly.  Rather than tune it
	// in 3 separate places again, I'll just limit it here.
	if ( length > 1500.0 )
	{
		vecNewVelocity *= 1500;		// This should really be sv_maxvelocity * 0.75 or something
		SetAbsVelocity( vecNewVelocity );
	}
}


void CGib::SpawnStickyGibs( CBaseEntity *pVictim, Vector vecOrigin, int cGibs )
{
	int i;

	if ( g_Language.GetInt() == LANGUAGE_GERMAN )
	{
		// no sticky gibs in germany right now!
		return; 
	}

	for ( i = 0 ; i < cGibs ; i++ )
	{
		CGib *pGib = (CGib *)CreateEntityByName( "gib" );

		pGib->Spawn( "models/stickygib.mdl" );
		pGib->m_nBody = random->RandomInt(0,2);

		if ( pVictim )
		{
			pGib->SetLocalOrigin(
				Vector( vecOrigin.x + random->RandomFloat( -3, 3 ),
						vecOrigin.y + random->RandomFloat( -3, 3 ),
						vecOrigin.z + random->RandomFloat( -3, 3 ) ) );

			// make the gib fly away from the attack vector
			Vector vecNewVelocity = g_vecAttackDir * -1;

			// mix in some noise
			vecNewVelocity.x += random->RandomFloat ( -0.15, 0.15 );
			vecNewVelocity.y += random->RandomFloat ( -0.15, 0.15 );
			vecNewVelocity.z += random->RandomFloat ( -0.15, 0.15 );

			vecNewVelocity *= 900;

			QAngle vecAngVelocity( random->RandomFloat ( 250, 400 ), random->RandomFloat ( 250, 400 ), 0 );
			pGib->SetLocalAngularVelocity( vecAngVelocity );

			// copy owner's blood color
			pGib->SetBloodColor( pVictim->BloodColor() );
		
			pGib->AdjustVelocityBasedOnHealth( pVictim->m_iHealth, vecNewVelocity );
			pGib->SetAbsVelocity( vecNewVelocity );
			
			pGib->SetMoveType( MOVETYPE_FLYGRAVITY );
			pGib->RemoveSolidFlags( FSOLID_NOT_SOLID );
			pGib->SetCollisionBounds( vec3_origin, vec3_origin );
			pGib->SetTouch ( &CGib::StickyGibTouch );
			pGib->SetThink (NULL);
		}
		pGib->LimitVelocity();
	}
}

void CGib::SpawnHeadGib( CBaseEntity *pVictim )
{
	CGib *pGib = CREATE_ENTITY( CGib, "gib" );

	if ( g_Language.GetInt() == LANGUAGE_GERMAN )
	{
		pGib->Spawn( "models/germangibs.mdl" );// throw one head
		pGib->m_nBody = 0;
	}
	else
	{
		pGib->Spawn( "models/gibs/hgibs.mdl" );// throw one head
		pGib->m_nBody = 0;
	}

	if ( pVictim )
	{
		Vector vecNewVelocity = pGib->GetAbsVelocity();

		pGib->SetLocalOrigin( pVictim->EyePosition() );
		
		edict_t *pentPlayer = UTIL_FindClientInPVS( pGib->edict() );
		
		if ( random->RandomInt ( 0, 100 ) <= 5 && pentPlayer )
		{
			// 5% chance head will be thrown at player's face.
			CBasePlayer *player = (CBasePlayer *)CBaseEntity::Instance( pentPlayer );
			if ( player )
			{
				vecNewVelocity = ( player->EyePosition() ) - pGib->GetAbsOrigin();
				VectorNormalize(vecNewVelocity);
				vecNewVelocity *= 300;
				vecNewVelocity.z += 100;
			}
		}
		else
		{
			vecNewVelocity = Vector (random->RandomFloat(-100,100), random->RandomFloat(-100,100), random->RandomFloat(200,300));
		}

		QAngle vecNewAngularVelocity = pGib->GetLocalAngularVelocity();
		vecNewAngularVelocity.x = random->RandomFloat ( 100, 200 );
		vecNewAngularVelocity.y = random->RandomFloat ( 100, 300 );
		pGib->SetLocalAngularVelocity( vecNewAngularVelocity );

		// copy owner's blood color
		pGib->SetBloodColor( pVictim->BloodColor() );
		pGib->AdjustVelocityBasedOnHealth( pVictim->m_iHealth, vecNewVelocity );
		pGib->SetAbsVelocity( vecNewVelocity );
	}
	pGib->LimitVelocity();
}


//-----------------------------------------------------------------------------
// Blood color (see BLOOD_COLOR_* macros in baseentity.h)
//-----------------------------------------------------------------------------
void CGib::SetBloodColor( int nBloodColor )
{
	m_bloodColor = nBloodColor;
}


//------------------------------------------------------------------------------
// A little piece of duplicated code
//------------------------------------------------------------------------------
void CGib::AdjustVelocityBasedOnHealth( int nHealth, Vector &vecVelocity )
{
	if ( nHealth > -50)
	{
		vecVelocity *= 0.7;
	}
	else if ( nHealth > -200)
	{
		vecVelocity *= 2;
	}
	else
	{
		vecVelocity *= 4;
	}
}


//------------------------------------------------------------------------------
// Purpose : Initialize a gibs position and velocity
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CGib::InitGib( CBaseEntity *pVictim, float fMinVelocity, float fMaxVelocity )
{
	// ------------------------------------------------------------------------
	// If have a pVictim spawn the gib somewhere in the pVictim's bounding volume
	// ------------------------------------------------------------------------
	if ( pVictim )
	{
		// Find a random position within the bounding box (add 1 to Z to get it out of the ground)
		Vector vecOrigin;
		pVictim->CollisionProp()->RandomPointInBounds( vec3_origin, Vector( 1, 1, 1 ), &vecOrigin );
		vecOrigin.z += 1.0f;
		SetAbsOrigin( vecOrigin );	

		// make the gib fly away from the attack vector
		Vector vecNewVelocity =	 g_vecAttackDir * -1;

		// mix in some noise
		vecNewVelocity.x += random->RandomFloat ( -0.25, 0.25 );
		vecNewVelocity.y += random->RandomFloat ( -0.25, 0.25 );
		vecNewVelocity.z += random->RandomFloat ( -0.25, 0.25 );

		vecNewVelocity *= random->RandomFloat ( fMaxVelocity, fMinVelocity );

		QAngle vecNewAngularVelocity = GetLocalAngularVelocity();
		vecNewAngularVelocity.x = random->RandomFloat ( 100, 200 );
		vecNewAngularVelocity.y = random->RandomFloat ( 100, 300 );
		SetLocalAngularVelocity( vecNewAngularVelocity );
		
		// copy owner's blood color
		SetBloodColor( pVictim->BloodColor() );
		
		AdjustVelocityBasedOnHealth( pVictim->m_iHealth, vecNewVelocity );

		// Attempt to be physical if we can
		if ( VPhysicsInitNormal( SOLID_BBOX, 0, false ) )
		{
			IPhysicsObject *pObj = VPhysicsGetObject();

			if ( pObj != NULL )
			{
				AngularImpulse angImpulse = RandomAngularImpulse( -500, 500 );
				pObj->AddVelocity( &vecNewVelocity, &angImpulse );
			}
		}
		else
		{
			SetSolid( SOLID_BBOX );
			SetCollisionBounds( vec3_origin, vec3_origin );
			SetAbsVelocity( vecNewVelocity );
		}
	
		SetCollisionGroup( COLLISION_GROUP_DEBRIS );
	}

	LimitVelocity();
}

//------------------------------------------------------------------------------
// Purpose : Given an .mdl file with gibs and the number of gibs in the file
//			 spawns them in pVictim's bounding box
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CGib::SpawnSpecificGibs(	CBaseEntity*	pVictim, 
								int				nNumGibs, 
								float			vMinVelocity, 
								float			vMaxVelocity, 
								const char*		cModelName,
								float			flLifetime)
{
	for (int i=0;i<nNumGibs;i++)
	{
		CGib *pGib = CREATE_ENTITY( CGib, "gib" );
		pGib->Spawn( cModelName );
		pGib->m_nBody = i;
		pGib->InitGib( pVictim, vMinVelocity, vMaxVelocity );
		pGib->m_lifeTime = flLifetime;
		
		if ( pVictim != NULL )
		{
			pGib->SetOwnerEntity( pVictim );
		}
	}
}

//------------------------------------------------------------------------------
// Purpose : Spawn random gibs of the given gib type
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CGib::SpawnRandomGibs( CBaseEntity *pVictim, int cGibs, GibType_e eGibType )
{
	int cSplat;

	for ( cSplat = 0 ; cSplat < cGibs ; cSplat++ )
	{
		CGib *pGib = CREATE_ENTITY( CGib, "gib" );

		if ( g_Language.GetInt() == LANGUAGE_GERMAN )
		{
			pGib->Spawn( "models/germangibs.mdl" );
			pGib->m_nBody = random->RandomInt(0,GERMAN_GIB_COUNT-1);
		}
		else
		{
			switch (eGibType)
			{
			case GIB_HUMAN:
				// human pieces
				pGib->Spawn( "models/gibs/hgibs.mdl" );
				pGib->m_nBody = random->RandomInt(1,HUMAN_GIB_COUNT-1);// start at one to avoid throwing random amounts of skulls (0th gib)
				break;
			case GIB_ALIEN:
				// alien pieces
				pGib->Spawn( "models/gibs/agibs.mdl" );
				pGib->m_nBody = random->RandomInt(0,ALIEN_GIB_COUNT-1);
				break;
			}
		}
		pGib->InitGib( pVictim, 300, 400);
	}
}

//=========================================================
// WaitTillLand - in order to emit their meaty scent from
// the proper location, gibs should wait until they stop 
// bouncing to emit their scent. That's what this function
// does.
//=========================================================
void CGib::WaitTillLand ( void )
{
	if (!IsInWorld())
	{
		UTIL_Remove( this );
		return;
	}

	if ( GetAbsVelocity() == vec3_origin )
	{
		SetRenderColorA( 255 );
		m_nRenderMode = kRenderTransTexture;
		if ( GetMoveType() != MOVETYPE_VPHYSICS )
		{
			AddSolidFlags( FSOLID_NOT_SOLID );
		}
		SetLocalAngularVelocity( vec3_angle );

		SetNextThink( gpGlobals->curtime + m_lifeTime );
		SetThink ( &CGib::SUB_FadeOut );

		if ( GetSprite() )
		{
			CSprite *pSprite = dynamic_cast<CSprite*>( GetSprite() );

			if ( pSprite )
			{
				//Adrian - Why am I doing this? Check InitPointGib for the answer!
				if ( m_lifeTime == 0 )
					m_lifeTime = random->RandomFloat( 1, 3 );

				pSprite->FadeAndDie( m_lifeTime );
			}
		}

		if ( GetFlame() )
		{
			CEntityFlame *pFlame = dynamic_cast< CEntityFlame*>( GetFlame() );

			if ( pFlame )
			{
				pFlame->SetLifetime( 1.0f );
			}
		}

		// If you bleed, you stink!
		if ( m_bloodColor != DONT_BLEED )
		{
			// ok, start stinkin!
			// FIXME: It's too easy to fill up the sound queue with all these meat sounds
			// CSoundEnt::InsertSound ( SOUND_MEAT, GetAbsOrigin(), 384, 25 );
		}
	}
	else
	{
		// wait and check again in another half second.
		SetNextThink( gpGlobals->curtime + 0.5f );
	}
}

bool CGib::SUB_AllowedToFade( void )
{
	if( VPhysicsGetObject() )
	{
		if( VPhysicsGetObject()->GetGameFlags() & FVPHYSICS_PLAYER_HELD || GetEFlags() & EFL_IS_BEING_LIFTED_BY_BARNACLE )
			return false;
	}

	CBasePlayer *pPlayer = ( AI_IsSinglePlayer() ) ? UTIL_GetLocalPlayer() : NULL;

	if ( pPlayer && pPlayer->FInViewCone( this ) && m_bForceRemove == false )
	{
		return false;
	}

	return true;
}


void CGib::DieThink ( void )
{
	if ( GetSprite() )
	{
		CSprite *pSprite = dynamic_cast<CSprite*>( GetSprite() );

		if ( pSprite )
		{
			pSprite->FadeAndDie( 0.0 );
		}
	}

	if ( GetFlame() )
	{
		CEntityFlame *pFlame = dynamic_cast< CEntityFlame*>( GetFlame() );

		if ( pFlame )
		{
			pFlame->SetLifetime( 1.0f );
		}
	}

	if ( g_pGameRules->IsMultiplayer() )
	{
		UTIL_Remove( this );
	}
	else
	{
		SetThink ( &CGib::SUB_FadeOut );
		SetNextThink( gpGlobals->curtime );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGib::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBasePlayer *pPlayer = ToBasePlayer( pActivator );

	if ( pPlayer )
	{
		pPlayer->PickupObject( this );
	}
}

//-----------------------------------------------------------------------------
// Physics Attacker
//-----------------------------------------------------------------------------
void CGib::SetPhysicsAttacker( CBasePlayer *pEntity, float flTime )
{
	m_hPhysicsAttacker = pEntity;
	m_flLastPhysicsInfluenceTime = flTime;
}

	
//-----------------------------------------------------------------------------
// Purpose: Keep track of physgun influence
//-----------------------------------------------------------------------------
void CGib::OnPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason )
{
	SetPhysicsAttacker( pPhysGunUser, gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGib::OnPhysGunDrop( CBasePlayer *pPhysGunUser, PhysGunDrop_t Reason )
{
	SetPhysicsAttacker( pPhysGunUser, gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CBasePlayer *CGib::HasPhysicsAttacker( float dt )
{
	if (gpGlobals->curtime - dt <= m_flLastPhysicsInfluenceTime)
	{
		return m_hPhysicsAttacker;
	}
	return NULL;
}


//
// Gib bounces on the ground or wall, sponges some blood down, too!
//
void CGib::BounceGibTouch ( CBaseEntity *pOther )
{
	Vector	vecSpot;
	trace_t	tr;
	
	IPhysicsObject *pPhysics = VPhysicsGetObject();

	if ( pPhysics )
		 return;
	
	//if ( random->RandomInt(0,1) )
	//	return;// don't bleed everytime
	if (GetFlags() & FL_ONGROUND)
	{
		SetAbsVelocity( GetAbsVelocity() * 0.9 );
		QAngle angles = GetLocalAngles();
		angles.x = 0;
		angles.z = 0;
		SetLocalAngles( angles );

		QAngle angVel = GetLocalAngularVelocity();
		angVel.x = 0;
		angVel.z = 0;
		SetLocalAngularVelocity( vec3_angle );
	}
	else
	{
		if ( g_Language.GetInt() != LANGUAGE_GERMAN && m_cBloodDecals > 0 && m_bloodColor != DONT_BLEED )
		{
			vecSpot = GetAbsOrigin() + Vector ( 0 , 0 , 8 );//move up a bit, and trace down.
			UTIL_TraceLine ( vecSpot, vecSpot + Vector ( 0, 0, -24 ),  MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr);

			UTIL_BloodDecalTrace( &tr, m_bloodColor );

			m_cBloodDecals--; 
		}

		if ( m_material != matNone && random->RandomInt(0,2) == 0 )
		{
			float volume;
			float zvel = fabs(GetAbsVelocity().z);
		
			volume = 0.8f * MIN(1.0, ((float)zvel) / 450.0f);

			CBreakable::MaterialSoundRandom( entindex(), (Materials)m_material, volume );
		}
	}
}

//
// Sticky gib puts blood on the wall and stays put. 
//
void CGib::StickyGibTouch ( CBaseEntity *pOther )
{
	Vector	vecSpot;
	trace_t tr;
	
	SetThink ( &CGib::SUB_Remove );
	SetNextThink( gpGlobals->curtime + 10 );

	if ( !FClassnameIs( pOther, "worldspawn" ) )
	{
		SetNextThink( gpGlobals->curtime );
		return;
	}

	UTIL_TraceLine ( GetAbsOrigin(), GetAbsOrigin() + GetAbsVelocity() * 32,  MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr);

	UTIL_BloodDecalTrace( &tr, m_bloodColor );

	Vector vecForward = tr.plane.normal * -1;
	QAngle angles;
	VectorAngles( vecForward, angles );
	SetLocalAngles( angles );
	SetAbsVelocity( vec3_origin ); 
	SetLocalAngularVelocity( vec3_angle );
	SetMoveType( MOVETYPE_NONE );
}

//
// Throw a chunk
//
void CGib::Spawn( const char *szGibModel )
{
	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );
	SetFriction(0.55); // deading the bounce a bit
	
	// sometimes an entity inherits the edict from a former piece of glass,
	// and will spawn using the same render FX or m_nRenderMode! bad!
	SetRenderColorA( 255 );
	m_nRenderMode = kRenderNormal;
	m_nRenderFX = kRenderFxNone;
	
	// hopefully this will fix the VELOCITY TOO LOW crap
	m_takedamage = DAMAGE_EVENTS_ONLY;
	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetCollisionGroup( COLLISION_GROUP_DEBRIS );

	SetModel( szGibModel );

#ifdef HL1_DLL
	SetElasticity( 1.0 );
	UTIL_SetSize( this, vec3_origin, vec3_origin );
#endif//HL1_DLL

	SetNextThink( gpGlobals->curtime + 4 );
	m_lifeTime = 25;
	SetTouch ( &CGib::BounceGibTouch );

    m_bForceRemove = false;

	m_material = matNone;
	m_cBloodDecals = 5;// how many blood decals this gib can place (1 per bounce until none remain). 

}


//-----------------------------------------------------------------------------
// Spawn a gib with a finite lifetime, after which it will fade out.
//-----------------------------------------------------------------------------
void CGib::Spawn( const char *szGibModel, float flLifetime )
{
	Spawn( szGibModel );
	m_lifeTime = flLifetime;
	SetThink ( &CGib::SUB_FadeOut );
	SetNextThink( gpGlobals->curtime + m_lifeTime );
}


LINK_ENTITY_TO_CLASS( gib, CGib );

CBaseEntity *CreateRagGib(const char *szModel, const Vector &vecOrigin, const QAngle &vecAngles, const Vector &vecForce, float flFadeTime, bool bShouldIgnite, int bloodColor)
{
	CRagGib *pGib;

	pGib = (CRagGib*)CreateEntityByName( "raggib" );

	pGib->SetLocalAngles( vecAngles );

	if ( !pGib )
	{
		Msg( "**Can't create ragdoll gib!\n" );
		return NULL;
	}

	pGib->Spawn(szModel, vecOrigin, vecForce, flFadeTime, bloodColor, bShouldIgnite);

	if (bShouldIgnite)
	{
		CBaseAnimating *pAnimating = pGib->GetBaseAnimating();
		if (pAnimating != NULL)
		{
			pAnimating->Ignite(random->RandomFloat(8.0, 12.0), false);
		}
	}

	return pGib;
}
CBaseEntity *CreateRagGibBodygrouped(const char *szModel, const Vector &vecOrigin, const QAngle &vecAngles, const Vector &vecForce, float flFadeTime, bool bShouldIgnite, int iGroup, int iValue, int bloodColor)
{
	CRagGib *pGib;

	pGib = (CRagGib*)CreateEntityByName("raggib");

	pGib->SetLocalAngles(vecAngles);

	if (!pGib)
	{
		Msg("**Can't create ragdoll gib!\n");
		return NULL;
	}

	pGib->SpawnBodygrouped(szModel, vecOrigin, vecForce, flFadeTime, iGroup, iValue, bloodColor, bShouldIgnite);

	if (bShouldIgnite)
	{
		CBaseAnimating *pAnimating = pGib->GetBaseAnimating();
		if (pAnimating != NULL)
		{
			pAnimating->Ignite(random->RandomFloat(8.0, 12.0), false);
		}
	}

	return pGib;
}
void CRagGib::Spawn(const char *szModel, const Vector &vecOrigin, const Vector &vecForce, float flFadeTime = 0.0, int bloodColor = -1, bool bShouldIgnite = false)
{
	SetSolid(SOLID_BBOX);
	//AddSolidFlags(FSOLID_NOT_SOLID);
	SetModel(szModel);
	UTIL_SetSize(this, vec3_origin, vec3_origin);
	UTIL_SetOrigin(this, vecOrigin);

	if (bShouldIgnite)
		Ignite(30.0f);

	if (IsOnFire() || cvar->FindVar("oc_ragdoll_serverside")->GetInt())
	{
		CTakeDamageInfo info;
		info.SetDamageForce(vecForce);
		BecomeRagdoll(info, vecForce, bloodColor, bShouldIgnite);
	}
	else
	{
		BecomeRagdollOnClient(vecForce);
		if (flFadeTime > 0.0)
		{
			SUB_StartFadeOut(flFadeTime);
		}
	}
}
void CRagGib::SpawnBodygrouped(const char *szModel, const Vector &vecOrigin, const Vector &vecForce, float flFadeTime = 0.0, int iGroup = 0, int iValue = 0, int bloodColor = -1, bool bShouldIgnite = false)
{
	SetSolid(SOLID_BBOX);
	SetModel(szModel);
	UTIL_SetSize(this, vec3_origin, vec3_origin);
	UTIL_SetOrigin(this, vecOrigin);

	if (bShouldIgnite)
		Ignite(30.0f);

	SetBodygroup(iGroup, iValue);

	if (IsOnFire() || cvar->FindVar("oc_ragdoll_serverside")->GetInt())
	{
		CTakeDamageInfo info;
		info.SetDamageForce(vecForce);
		BecomeRagdoll(info, vecForce, bloodColor, bShouldIgnite);
	}
	else
	{
		BecomeRagdollOnClient(vecForce);
		if (flFadeTime > 0.0)
		{
			SUB_StartFadeOut(flFadeTime);
		}
	}
}
void CRagGib::BecomeRagdoll(const CTakeDamageInfo &info, const Vector &forceVector, int bloodColor, bool bShouldIgnite)
{
	ConVarRef oc_ragdoll_serverside_collision_group("oc_ragdoll_serverside_collision_group");

	Collision_Group_t e_CGT;

	switch (oc_ragdoll_serverside_collision_group.GetInt())
	{
	case 0:
		e_CGT = COLLISION_GROUP_INTERACTIVE_DEBRIS;
		break;
	case 1:
		e_CGT = COLLISION_GROUP_INTERACTIVE;
		break;
	case 2:
		e_CGT = COLLISION_GROUP_NPC;
		break;
	default:
		e_CGT = COLLISION_GROUP_INTERACTIVE_DEBRIS;
		break;
	}

	CBaseEntity *pRagdoll = CreateServerRagdoll(this, m_nForceBone, info, e_CGT, true, bloodColor);

	if (bShouldIgnite)
	{
		CBaseAnimating *pAnimating = pRagdoll->GetBaseAnimating();
		if (pAnimating != NULL)
		{
			pAnimating->Ignite(random->RandomFloat(8.0, 12.0), false);
		}
	}

	FixupBurningServerRagdoll(pRagdoll);

	SetupServerRagdoll(pRagdoll);

	RemoveDeferred();

	if (!(GetFlags() & FL_TRANSRAGDOLL))
	{
		RemoveDeferred();
	}
}

void CRagGib::FixupBurningServerRagdoll(CBaseEntity *pRagdoll, bool bShouldIgnite)
{
	if (!IsOnFire())
		return;

	// Move the fire effects entity to the ragdoll
	CEntityFlame *pFireChild = dynamic_cast<CEntityFlame *>(GetEffectEntity());
	if (pFireChild)
	{
		SetEffectEntity(NULL);
		pRagdoll->AddFlag(FL_ONFIRE);
		pFireChild->SetAbsOrigin(pRagdoll->GetAbsOrigin());
		pFireChild->AttachToEntity(pRagdoll);
		pFireChild->AddEFlags(EFL_FORCE_CHECK_TRANSMIT);
		pRagdoll->SetEffectEntity(pFireChild);

		color32 color = GetRenderColor();
		pRagdoll->SetRenderColor(color.r, color.g, color.b);
		pFireChild->SetLifetime(10.0f);
	}
}

void CRagGib::SetupServerRagdoll(CBaseEntity *pRagdoll)
{
	ConVarRef oc_ragdoll_serverside_collision_type("oc_ragdoll_serverside_collision_type");

	switch (oc_ragdoll_serverside_collision_type.GetInt())
	{
	case 0:
	{
		PhysSetEntityGameFlags(pRagdoll, FVPHYSICS_NO_SELF_COLLISIONS);
	}
	break;
	case 1:
	{
		PhysSetEntityGameFlags(pRagdoll, FVPHYSICS_PART_OF_RAGDOLL);
	}
	break;
	case 2:
	{
		PhysSetEntityGameFlags(pRagdoll, FVPHYSICS_MULTIOBJECT_ENTITY);
	}
	break;
	case 3:
	{
		PhysSetEntityGameFlags(pRagdoll, FVPHYSICS_HEAVY_OBJECT);
	}
	break;
	case 4:
	{
		PhysSetEntityGameFlags(pRagdoll, FVPHYSICS_NO_NPC_IMPACT_DMG);
	}
	break;
	case 5:
	{
		PhysSetEntityGameFlags(pRagdoll, FVPHYSICS_NO_IMPACT_DMG);
	}
	break;
	case 6:
	{
		PhysSetEntityGameFlags(pRagdoll, FVPHYSICS_WAS_THROWN);
	}
	break;
	default:
	{
		PhysSetEntityGameFlags(pRagdoll, FVPHYSICS_PART_OF_RAGDOLL);
	}
	break;
	}
}

LINK_ENTITY_TO_CLASS( raggib, CRagGib );
