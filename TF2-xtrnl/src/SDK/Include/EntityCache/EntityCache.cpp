#include "EntityCache.h"
#include "../Client/Client.h"

void CEntityCache::Fill()
{
	CEntity _Local = g_Client.GetLocalPlayer();

	if (!_Local || !_Local.IsInValidTeam())
		return;

	m_Local = _Local;

	for (int n = 1; n < 32; n++)
	{
		CEntity Entity = g_EntityList.GetEntity(n);

		if (!Entity)
			continue;

		switch (Entity.GetClassID())
		{
			case CTFPlayer:
			{
				if (!Entity.IsInValidTeam())
					continue;

				m_Groups[EGroupType::PLAYERS_ALL].push_back(Entity);
				m_Groups[Entity.GetTeamNum() != m_Local.GetTeamNum() ? EGroupType::PLAYERS_ENEMIES : EGroupType::PLAYERS_TEAMMATES].push_back(Entity);
				break;
			}

			default: break;
		}
	}
}

void CEntityCache::Clear()
{
	m_Local = 0x0;

	for (auto &Group : m_Groups)
		Group.second.clear();
}

const std::vector<CEntity> &CEntityCache::GetGroup(EGroupType Group) {
	return m_Groups[Group];
}