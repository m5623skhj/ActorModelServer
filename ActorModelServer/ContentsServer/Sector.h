#pragma once

class Sector
{
public:
	Sector() = default;
	~Sector() = default;
	Sector(const Sector&) = delete;
	Sector& operator=(const Sector&) = delete;
	Sector& operator=(Sector&&) noexcept = default;
	Sector(Sector&&) noexcept = default;
};