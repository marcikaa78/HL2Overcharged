#include	"cbase.h"
#include	"AI_Default.h"
#include	"AI_Task.h"
#include	"AI_Schedule.h"
#include	"AI_Node.h"
#include	"AI_Hull.h"
#include	"AI_Hint.h"
#include	"AI_Memory.h"
#include	"AI_Route.h"
#include	"AI_Senses.h"
#include	"soundent.h"
#include	"game.h"
#include	"NPCEvent.h"
#include	"EntityList.h"
#include	"activitylist.h"
#include	"animation.h"
#include	"basecombatweapon.h"
#include	"IEffects.h"
#include	"vstdlib/random.h"
#include	"engine/IEngineSound.h"
#include	"ammodef.h"
#include    "te.h"
#include    "hl2_shareddefs.h"
#include    "projectile_hornet.h"

int iHornetTrail;
int iHornetPuff;

//extern ConVar oc_weapon_ShockRifle_model;
#define HORNET_MODEL	"models/weapons/bee.mdl"//oc_weapon_ShockRifle_model.GetString() //"models/ShockRifle.mdl"

LINK_ENTITY_TO_CLASS(hornet, CNPC_Hornet);

extern ConVar sk_npc_dmg_hornet;// ("sk_npc_dmg_hornet", "5");
extern ConVar sk_plr_dmg_hornet;// ("sk_plr_dmg_hornet", "15");

BEGIN_DATADESC(CNPC_Hornet)
DEFINE_FIELD(m_flStopAttack, FIELD_TIME),
DEFINE_FIELD(m_iHornetType, FIELD_INTEGER),
DEFINE_FIELD(m_flFlySpeed, FIELD_FLOAT),
DEFINE_FIELD(m_hParticleTrail, FIELD_EHANDLE),
DEFINE_ENTITYFUNC(DieTouch),
DEFINE_THINKFUNC(StartDart),
DEFINE_THINKFUNC(StartTrack),
DEFINE_ENTITYFUNC(DartTouch),
DEFINE_ENTITYFUNC(TrackTouch),
DEFINE_THINKFUNC(TrackTarget),
END_DATADESC()

//=========================================================
//=========================================================
void CNPC_Hornet::Spawn(void)
{
	Precache();

	SetMoveType(MOVETYPE_FLY);
	SetSolid(SOLID_BBOX);
	m_takedamage = DAMAGE_YES;
	AddFlag(FL_NPC);
	m_iHealth = 1;// weak!

	CreateSprites();

	if (g_pGameRules->IsMultiplayer())
	{
		// hornets don't live as long in multiplayer
		m_flStopAttack = gpGlobals->curtime + 3.5;
	}
	else
	{
		m_flStopAttack = gpGlobals->curtime + 7.0;
	}

	m_flFieldOfView = 0.5; // +- 25 degrees //0.9

	if (random->RandomInt(1, 5) <= 2)
	{
		m_iHornetType = HORNET_TYPE_RED;
		m_flFlySpeed = HORNET_RED_SPEED;
	}
	else
	{
		m_iHornetType = HORNET_TYPE_ORANGE;
		m_flFlySpeed = HORNET_ORANGE_SPEED;
	}

	//SetModel("models/hornet.mdl");
	SetModel(HORNET_MODEL);
	UTIL_SetSize(this, Vector(-4, -4, -4), Vector(4, 4, 4));

	SetTouch(&CNPC_Hornet::DieTouch);
	SetThink(&CNPC_Hornet::StartTrack);

	if (GetOwnerEntity() && (GetOwnerEntity()->GetFlags() & FL_CLIENT))
	{
		m_flDamage = sk_plr_dmg_hornet.GetFloat();
	}
	else
	{
		// no real owner, or owner isn't a client. 
		m_flDamage = sk_npc_dmg_hornet.GetFloat();
	}

	SetNextThink(gpGlobals->curtime + 0.1f);
	ResetSequenceInfo();
}

void CNPC_Hornet::Precache()
{
	engine->PrecacheModel("models/weapons/bee.mdl");
	PrecacheModel(HORNET_MODEL);
	enginesound->PrecacheSound("hornet/ag_buzz1.wav");
	enginesound->PrecacheSound("hornet/ag_buzz2.wav");
	enginesound->PrecacheSound("hornet/ag_buzz3.wav");

	enginesound->PrecacheSound("hornet/ag_hornethit1.wav");
	enginesound->PrecacheSound("hornet/ag_hornethit2.wav");
	enginesound->PrecacheSound("hornet/ag_hornethit3.wav");

	PrecacheParticleSystem("blood_trail_alien");
	PrecacheParticleSystem("grenade_spit");
	PrecacheParticleSystem("grenade_spit_trail");
	PrecacheParticleSystem("blood_spit_trail_green");
	PrecacheParticleSystem("blood_green_normal_smoke");
	PrecacheParticleSystem("blood_drip_green_trail");

	PrecacheParticleSystem("hornet_trail");

	iHornetPuff = engine->PrecacheModel("sprites/muz1.vmt");
	iHornetTrail = engine->PrecacheModel("sprites/laserbeam.vmt");
}

void CNPC_Hornet::CreateSprites(void)
{
	if (m_hParticleTrail == NULL)
	{
		m_hParticleTrail = (CParticleSystem *)CreateEntityByName("info_particle_system");
		if (m_hParticleTrail != NULL)
		{
			// Setup our basic parameters
			m_hParticleTrail->KeyValue("start_active", "1");
			m_hParticleTrail->KeyValue("effect_name", "hornet_trail");
			m_hParticleTrail->SetParent(this);
			m_hParticleTrail->SetLocalOrigin(vec3_origin);
			DispatchSpawn(m_hParticleTrail);
			if (gpGlobals->curtime > 0.5f)
				m_hParticleTrail->Activate();
		}
	}
}
//=========================================================
// hornets will never get mad at each other, no matter who the owner is.
//=========================================================
Disposition_t CNPC_Hornet::IRelationType(CBaseEntity *pTarget)
{
	if (pTarget->GetModelIndex() == GetModelIndex())
	{
		return D_NU;
	}

	return BaseClass::IRelationType(pTarget);
}

//=========================================================
// ID's Hornet as their owner
//=========================================================
Class_T CNPC_Hornet::Classify(void)
{
	if (GetOwnerEntity() && (GetOwnerEntity()->GetFlags() & FL_CLIENT))
	{
		return CLASS_BEE_PLR;
	}

	return	CLASS_BEE;
}

//=========================================================
// StartDart - starts a hornet out just flying straight.
//=========================================================
void CNPC_Hornet::StartDart(void)
{
	IgniteTrail();

	SetTouch(&CNPC_Hornet::DartTouch);

	SetThink(&CNPC_Hornet::SUB_Remove);
	SetNextThink(gpGlobals->curtime + 4);
}


void CNPC_Hornet::DieTouch(CBaseEntity *pOther)
{
	if (pOther && pOther->GetOwnerEntity() == this->GetOwnerEntity())
		return;

	if (pOther && (!pOther->IsSolid() || pOther->IsSolidFlagSet(FSOLID_VOLUME_CONTENTS)))
	{
		if (pOther->GetOwnerEntity() && this->GetOwnerEntity())
		{
			if (pOther->GetOwnerEntity() != this->GetOwnerEntity())
			{

			}
		}
		else
			return;
	}

	/*CPASAttenuationFilter filter(this);
	switch (random->RandomInt(0, 2))
	{// buzz when you plug someone
	case 0:	enginesound->EmitSound(filter, entindex(), CHAN_VOICE, "hornet/ag_hornethit1.wav", 1, ATTN_NORM);	break;
	case 1:	enginesound->EmitSound(filter, entindex(), CHAN_VOICE, "hornet/ag_hornethit2.wav", 1, ATTN_NORM);	break;
	case 2:	enginesound->EmitSound(filter, entindex(), CHAN_VOICE, "hornet/ag_hornethit3.wav", 1, ATTN_NORM);	break;
	}*/


	trace_t tr;

	AI_TraceLine(GetAbsOrigin(), GetAbsOrigin() + GetAbsVelocity(), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);

	const trace_t *pTrace = &tr;

	bool bHitWater = ((pTrace->contents & CONTENTS_WATER) != 0);

	const Vector tracePlaneNormal = pTrace->plane.normal;

	if (bHitWater)
	{
		// Splash!
		CEffectData data;
		data.m_fFlags = 0;
		data.m_vOrigin = pTrace->endpos;
		data.m_vNormal = Vector(0, 0, 1);
		data.m_flScale = 8.0f;

		DispatchEffect("watersplash", data);
	}
	else
	{
		// Make a splat decal
		UTIL_DecalTrace(pTrace, "YellowBlood");
	}


	if (pOther)
	{
		CTakeDamageInfo info(this, GetOwnerEntity(), m_flDamage, DMG_BULLET);
		CalculateBulletDamageForce(&info, GetAmmoDef()->Index("Pistol"), GetAbsVelocity(), GetAbsOrigin());
		pOther->TakeDamage(info);
	}

	m_takedamage = DAMAGE_NO;

	AddEffects(EF_NODRAW);

	AddSolidFlags(FSOLID_NOT_SOLID);// intangible
	//UTIL_Relink(this);

	UTIL_Remove(this);
	SetTouch(NULL);
}


//=========================================================
// StartTrack - starts a hornet out tracking its target
//=========================================================
void CNPC_Hornet::StartTrack(void)
{
	IgniteTrail();

	SetTouch(&CNPC_Hornet::TrackTouch);
	SetThink(&CNPC_Hornet::TrackTarget);

	SetNextThink(gpGlobals->curtime + 0.1f);

	if (GetAbsVelocity().Length() <= 0)
		DieTouch(NULL);
}

void TE_BeamFollow(IRecipientFilter& filter, float delay,
	int iEntIndex, int modelIndex, int haloIndex, float life, float width, float endWidth,
	float fadeLength, float r, float g, float b, float a);

void CNPC_Hornet::IgniteTrail(void)
{
	Vector vColor;

	if (m_iHornetType == HORNET_TYPE_RED)
		vColor = Vector(179, 39, 14);
	else
		vColor = Vector(255, 128, 0);

	CBroadcastRecipientFilter filter;
	TE_BeamFollow(filter, 0.0,
		entindex(),
		iHornetTrail,
		0,
		1,
		2,
		0.5,
		0.5,
		vColor.x,
		vColor.y,
		vColor.z,
		128);
}

//=========================================================
// Tracking Hornet hit something
//=========================================================
void CNPC_Hornet::TrackTouch(CBaseEntity *pOther)
{
	if (!pOther->IsSolid() || pOther->IsSolidFlagSet(FSOLID_VOLUME_CONTENTS))
	{
		return;
	}

	if (pOther == GetOwnerEntity() || pOther->GetModelIndex() == GetModelIndex())
	{// bumped into the guy that shot it.
		//SetSolid( SOLID_NOT );
		return;
	}

	if (pOther->GetOwnerEntity() && this->GetOwnerEntity() && pOther->GetOwnerEntity() == this->GetOwnerEntity())
		DieTouch(pOther);

	int nRelationship = IRelationType(pOther);
	if ((nRelationship == D_FR || nRelationship == D_NU || nRelationship == D_LI))
	{
		// hit something we don't want to hurt, so turn around.
		Vector vecVel = GetAbsVelocity();

		VectorNormalize(vecVel);

		vecVel.x *= -1;
		vecVel.y *= -1;

		SetAbsOrigin(GetAbsOrigin() + vecVel * 4); // bounce the hornet off a bit.
		SetAbsVelocity(vecVel * m_flFlySpeed);





		trace_t tr;
		AI_TraceLine(GetAbsOrigin(), GetAbsOrigin() + GetAbsVelocity(), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);

		const trace_t *pTrace = &tr;

		bool bHitWater = ((pTrace->contents & CONTENTS_WATER) != 0);

		const Vector tracePlaneNormal = pTrace->plane.normal;

		if (bHitWater)
		{
			// Splash!
			CEffectData data;
			data.m_fFlags = 0;
			data.m_vOrigin = pTrace->endpos;
			data.m_vNormal = Vector(0, 0, 1);
			data.m_flScale = 8.0f;

			DispatchEffect("watersplash", data);
		}
		else
		{
			// Make a splat decal
			UTIL_DecalTrace(pTrace, "YellowBlood");
		}

		return;
	}

	DieTouch(pOther);
}

void CNPC_Hornet::DartTouch(CBaseEntity *pOther)
{
	DieTouch(pOther);
}

float m_iBeeFlyOffset;
//=========================================================
// Hornet is flying, gently tracking target
//=========================================================
void CNPC_Hornet::TrackTarget(void)
{
	Vector	vecFlightDir;
	Vector	vecDirToEnemy;
	float	flDelta;
	Vector  vecEnemyLKP;

	//DevMsg("GetAbsVelocity: %.2f %.2f %.2f \n", GetAbsVelocity().x, GetAbsVelocity().y, GetAbsVelocity().z);

	StudioFrameAdvance();

	if (gpGlobals->curtime > m_flStopAttack)
	{
		SetTouch(NULL);
		SetThink(&CNPC_Hornet::SUB_Remove);
		SetNextThink(gpGlobals->curtime + 0.1f);
		return;
	}
	// UNDONE: The player pointer should come back after returning from another level
	if (GetEnemy() == NULL)
	{// enemy is dead.
		GetSenses()->Look(512);
		SetEnemy(BestEnemy());
	}

	if (GetEnemy() != NULL && FVisible(GetEnemy()))
	{
		vecEnemyLKP = GetEnemy()->BodyTarget(GetAbsOrigin());
	}
	else
	{
		//Vector EnemyLastPos = GetEnemyLKP();
		//VectorNormalize(EnemyLastPos);
		vecEnemyLKP = /*EnemyLastPos + */GetAbsVelocity() * m_flFlySpeed * 0.1;
	}

	vecDirToEnemy = vecEnemyLKP - GetAbsOrigin();
	VectorNormalize(vecDirToEnemy);

	//DevMsg("vecEnemyLKP: %.2f %.2f %.2f \n", vecEnemyLKP.x, vecEnemyLKP.y, vecEnemyLKP.z);
	//DevMsg("vecDirToEnemy: %.2f %.2f %.2f \n", vecDirToEnemy.x, vecDirToEnemy.y, vecDirToEnemy.z);

	if (GetEnemy() != NULL && FVisible(GetEnemy()))
	{
		vecFlightDir = vecDirToEnemy;
	}
	else
	{
		vecFlightDir = GetAbsVelocity();
		VectorNormalize(vecFlightDir);
	}
	/*if (GetAbsVelocity().Length() < 0.1)
		vecFlightDir = vecDirToEnemy;
	else
	{
		vecFlightDir = GetAbsVelocity();
		VectorNormalize(vecFlightDir);
	}*/

	//DevMsg("vecFlightDir: %.2f %.2f %.2f \n", vecFlightDir.x, vecFlightDir.y, vecFlightDir.z);

	SetAbsVelocity(vecFlightDir + vecDirToEnemy);

	// measure how far the turn is, the wider the turn, the slow we'll go this time.
	flDelta = DotProduct(vecFlightDir, vecDirToEnemy);

	//DevMsg("flDelta: %.2f \n", flDelta);


	if (flDelta < 0.5)
	{// hafta turn wide again. play sound
		CPASAttenuationFilter filter(this);
		switch (random->RandomInt(0, 2))
		{
		case 0:	enginesound->EmitSound(filter, entindex(), CHAN_VOICE, "hornet/ag_buzz1.wav", HORNET_BUZZ_VOLUME, ATTN_NORM);	break;
		case 1:	enginesound->EmitSound(filter, entindex(), CHAN_VOICE, "hornet/ag_buzz2.wav", HORNET_BUZZ_VOLUME, ATTN_NORM);	break;
		case 2:	enginesound->EmitSound(filter, entindex(), CHAN_VOICE, "hornet/ag_buzz3.wav", HORNET_BUZZ_VOLUME, ATTN_NORM);	break;
		}
	}

	if (flDelta <= 0 && m_iHornetType == HORNET_TYPE_RED)
	{// no flying backwards, but we don't want to invert this, cause we'd go fast when we have to turn REAL far.
		flDelta = 0.5;
	}

	Vector vecVel = GetAbsVelocity();
	VectorNormalize(vecVel);

	//if (GetOwnerEntity() && (GetOwnerEntity()->GetFlags() & FL_NPC))
	/*{
	// random pattern only applies to hornets fired by monsters, not players.

	vecVel.x += random->RandomFloat(-0.10, 0.10);// scramble the flight dir a bit.
	vecVel.y += random->RandomFloat(-0.10, 0.10);
	vecVel.z += random->RandomFloat(-0.10, 0.10);
	}*/
	//DevMsg("SetAbsVelocity: %.2f %.2f %.2f \n", vecVel.x, vecVel.y, vecVel.z);
	SetAbsVelocity(vecVel);
	//DevMsg("flDelta: %.2f\n", flDelta);
	switch (m_iHornetType)
	{
	case HORNET_TYPE_RED:
		//DevMsg("SetAbsVelocity: %.2f %.2f %.2f \n", vecVel.x, vecVel.y, vecVel.z);
		SetAbsVelocity(GetAbsVelocity() * (m_flFlySpeed * flDelta));// scale the dir by the ( speed * width of turn )
		SetNextThink(gpGlobals->curtime + random->RandomFloat(0.1f));
		break;
	case HORNET_TYPE_ORANGE:
		//DevMsg("SetAbsVelocity: %.2f %.2f %.2f \n", vecVel.x, vecVel.y, vecVel.z);
		SetAbsVelocity(GetAbsVelocity() * m_flFlySpeed);// do not have to slow down to turn.
		SetNextThink(gpGlobals->curtime + 0.1f);// fixed think time
		break;
	}

	QAngle angNewAngles;
	VectorAngles(GetAbsVelocity(), angNewAngles);
	SetAbsAngles(angNewAngles);

	SetSolid(SOLID_BBOX);

	if (GetAbsVelocity().Length() <= 300)
		DieTouch(NULL);

	/*if (GetAbsVelocity().Length() <= 0.1f)
		DieTouch(GetEnemy());*/
	// if hornet is close to the enemy, jet in a straight line for a half second.
	// (only in the single player game)
	if (GetEnemy() != NULL && !g_pGameRules->IsMultiplayer())
	{
		if (flDelta >= 0.4 && (GetAbsOrigin() - vecEnemyLKP).Length() <= 300)
		{
			CPVSFilter filter(GetAbsOrigin());
			te->Sprite(filter, 0.0,
				&GetAbsOrigin(), // pos
				iHornetPuff,	// model
				0.2,				//size
				128				// brightness
				);

			CPASAttenuationFilter filter2(this);
			switch (random->RandomInt(0, 2))
			{
			case 0:	enginesound->EmitSound(filter2, entindex(), CHAN_VOICE, "hornet/ag_buzz1.wav", HORNET_BUZZ_VOLUME, ATTN_NORM);	break;
			case 1:	enginesound->EmitSound(filter2, entindex(), CHAN_VOICE, "hornet/ag_buzz2.wav", HORNET_BUZZ_VOLUME, ATTN_NORM);	break;
			case 2:	enginesound->EmitSound(filter2, entindex(), CHAN_VOICE, "hornet/ag_buzz3.wav", HORNET_BUZZ_VOLUME, ATTN_NORM);	break;
			}
			SetAbsVelocity(GetAbsVelocity() * 2);
			SetNextThink(gpGlobals->curtime + 1.0f);
			// don't attack again
			m_flStopAttack = gpGlobals->curtime;
		}
	}
}