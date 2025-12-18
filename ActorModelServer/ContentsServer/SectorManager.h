#pragma once
#include <unordered_map>
#include "Sector.h"

class SectorManager
{
private:
	SectorManager() = default;
	~SectorManager() = default;

public:
	static SectorManager& GetInst()
	{
		static SectorManager instance;
		return instance;
	}

	SectorManager(const SectorManager&) = delete;
	SectorManager& operator=(const SectorManager&) = delete;

public:
	bool RegisterSector(short sectorId, Sector&& sector);
	void UnregisterSector(short sectorId);
	const Sector* GetSector(short sectorId);

private:
	std::unordered_map<short, Sector> sectorMap;
};