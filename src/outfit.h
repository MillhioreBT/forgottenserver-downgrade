// Copyright 2023 The Forgotten Server Authors. All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#ifndef FS_OUTFIT_H
#define FS_OUTFIT_H

#include "enums.h"

struct Outfit
{
	Outfit(std::string_view name, uint16_t lookType, PlayerSex_t sex, bool premium, bool unlocked) :
	    name{name}, lookType{lookType}, sex{sex}, premium{premium}, unlocked{unlocked}
	{}

	bool operator==(const Outfit& otherOutfit) const
	{
		return name == otherOutfit.name && lookType == otherOutfit.lookType && sex == otherOutfit.sex &&
		       premium == otherOutfit.premium && unlocked == otherOutfit.unlocked;
	}

	std::string name;
	uint16_t lookType;
	PlayerSex_t sex;
	bool premium;
	bool unlocked;
};

struct ProtocolOutfit
{
	ProtocolOutfit(std::string_view name, uint16_t lookType, uint8_t addons) :
	    name{name}, lookType{lookType}, addons{addons}
	{}

	std::string name;
	uint16_t lookType;
	uint8_t addons;
};

class Outfits
{
public:
	static Outfits& getInstance()
	{
		static Outfits instance;
		return instance;
	}

	bool loadFromXml();

	const Outfit* getOutfitByLookType(uint16_t lookType) const;
	const std::vector<const Outfit*> getOutfits(PlayerSex_t sex) const;

private:
	std::unordered_map<uint16_t, Outfit> outfits;
};

#endif
