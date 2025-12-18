#include "PreCompile.h"
#include "SectorManager.h"

bool SectorManager::RegisterSector(short sectorId, Sector&& sector)
{
	sectorMap.emplace(sectorId, std::move(sector));
	return true;
}

void SectorManager::UnregisterSector(const short sectorId)
{
	sectorMap.erase(sectorId);
}

const Sector* SectorManager::GetSector(const short sectorId)
{
	if (const auto it = sectorMap.find(sectorId); it != sectorMap.end())
	{
		return &it->second;
	}

	return nullptr;
}
