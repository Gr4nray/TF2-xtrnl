#include "Aimbot.h"
#include "../Vars.h"

bool CAimbot::IsKeyDown() {
	return !Vars::Aimbot::AimKey ? true : (GetAsyncKeyState(Vars::Aimbot::AimKey) & 0x8000);
}

bool CAimbot::GetTargets(const CEntity &Local)
{
	m_vecTargets.clear();

	Vector vViewAngles = g_Engine.GetViewAngles();
	Vector vLocalEyePos = Local.GetEyePosition();
	Vector vLocalOrigin = Local.GetOrigin();

	if (Vars::Aimbot::AimPlayers)
	{
		for (const auto &Player : g_EntityCache.GetGroup(EGroupType::PLAYERS_ENEMIES))
		{
			Vector vPos = GetAimPosition(Player, true);

			//took a while to realize this, it almost drove me crazy
			//it used to aim at "dead players" (and if glow was enabled it would fix itself SMH)
			//then I found out that it was GetBonePos and that this is one way to fix it
			if (vPos.DistTo(Player.GetOrigin()) > 100.0f)
				continue;

			Vector vAngleTo = Math::CalcAngle(vLocalEyePos, vPos);
			float flFOVTo = Vars::Aimbot::SortMethod == 0 ? Math::CalcFov(vViewAngles, vAngleTo) : 0.0f;
			float flDistTo = Vars::Aimbot::SortMethod == 1 ? vLocalOrigin.DistTo(vPos) : 0.0f;

			if (Vars::Aimbot::SortMethod == 0 && flFOVTo > Vars::Aimbot::AimFOV)
				continue;

			m_vecTargets.push_back({ Player.GetThis(), vPos, vAngleTo, flFOVTo, flDistTo });
		}
	}

	if (Vars::Aimbot::AimBuildings)
	{
		for (const auto &Building : g_EntityCache.GetGroup(EGroupType::BUILDINGS_ENEMIES))
		{
			Vector vPos = GetAimPosition(Building, false);
			Vector vAngleTo = Math::CalcAngle(vLocalEyePos, vPos);
			float flFOVTo = Vars::Aimbot::SortMethod == 0 ? Math::CalcFov(vViewAngles, vAngleTo) : 0.0f;
			float flDistTo = Vars::Aimbot::SortMethod == 1 ? vLocalOrigin.DistTo(vPos) : 0.0f;

			if (Vars::Aimbot::SortMethod == 0 && flFOVTo > Vars::Aimbot::AimFOV)
				continue;

			m_vecTargets.push_back({ Building.GetThis(), vPos, vAngleTo, flFOVTo, flDistTo });
		}
	}

	return !m_vecTargets.empty();
}

bool CAimbot::GetTarget(const CEntity &Local, Target_t &TargetOut)
{
	if (!GetTargets(Local))
		return false;

	std::sort(m_vecTargets.begin(), m_vecTargets.end(), [&](const Target_t &a, const Target_t &b) -> bool
	{
		switch (Vars::Aimbot::SortMethod) {
			case 0: return a.m_flFOVTo < b.m_flFOVTo;
			case 1: return a.m_flDistTo < b.m_flDistTo;
			default: return a.m_flDistTo < b.m_flDistTo;
		}
	});

	TargetOut = m_vecTargets[0];

	return true;
}

Vector CAimbot::GetAimPosition(const CEntity &Entity, bool bIsPlayer)
{
	if (bIsPlayer)
	{
		auto GetHeadPos = [&]() -> Vector
		{
			switch (Entity.GetClassNum())
			{
				case CLASS_SCOUT:
				case CLASS_SOLDIER:
				case CLASS_PYRO:
				case CLASS_HEAVY:
				case CLASS_MEDIC:
				case CLASS_SNIPER:
				case CLASS_SPY: return Entity.GetBonePos(6) + Vector(0.0f, 0.0f, 4.5f);
				case CLASS_DEMOMAN: return Entity.GetBonePos(16) + Vector(0.0f, 0.0f, 4.5f);
				case CLASS_ENGINEER: return Entity.GetBonePos(8) + Vector(0.0f, 0.0f, 4.5f);
				default: return Vector();
			}
		};

		auto GetBodyPos = [&]() -> Vector {
			return Entity.GetBonePos(0);
		};

		switch (Vars::Aimbot::AimPosition)
		{
			case 0: return GetBodyPos();
			case 1: return GetHeadPos();
			case 2:
			{
				CEntity Local = g_EntityCache.m_Local;

				switch (Local.GetClassNum())
				{
					case CLASS_SNIPER: return Local.IsScoped() ? GetHeadPos() : GetBodyPos();
					case CLASS_SPY:
					{
						return GetHeadPos();

						/* doesn't work?? :((
						switch (CEntity(g_EntityList.GetEntityFromHandle(Local.GetActiveWeapon())).GetItemDefinitionIndex()) {
							case Spy_m_TheAmbassador:
							case Spy_m_FestiveAmbassador: return GetHeadPos();
							default: return GetBodyPos();
						}
						*/
					}

					default: return GetBodyPos();
				}
			}
		}
	}

	return Entity.GetOrigin() + Vector(0.0f, 0.0f, Entity.IsTeleporter() ? 5.0f : 25.0f);
}

void CAimbot::Aim(Vector &vAngles)
{
	switch (Vars::Aimbot::AimMethod)
	{
		case 0: {
			g_Engine.SetViewAngles(vAngles);
			break;
		}

		case 1: {
			Vector vViewAngles = g_Engine.GetViewAngles();
			Vector vDelta = vAngles - vViewAngles;
			Math::ClampAngles(vDelta);
			vViewAngles += vDelta / (Vars::Aimbot::Smoothing * 10.0f);
			g_Engine.SetViewAngles(vViewAngles);
			break;
		}

		default: break;
	}
}

void CAimbot::Run()
{
	if (!Vars::Aimbot::Active)
		return;
	
	if (const auto &Local = g_EntityCache.m_Local)
	{
		if (!Local.IsAlive())
			return;
		
		Target_t Target = {};
		
		if (GetTarget(Local, Target) && IsKeyDown())
		{
			Aim(Target.m_vAngleTo);
			
			if (Vars::Aimbot::AutoShoot)
				g_Client.SetAttack(6);
		}
	}
}