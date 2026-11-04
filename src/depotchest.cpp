// Copyright 2023 The Forgotten Server Authors. All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include "depotchest.h"

#include "tools.h"

DepotChest::DepotChest(uint16_t type) : Container(type) {}

ReturnValue DepotChest::queryAdd(int32_t index, const Thing& thing, uint32_t count, uint32_t flags,
                                 Creature* actor /* = nullptr*/) const
{
	const Item* item = thing.getItem();
	if (item == nullptr) {
		return RETURNVALUE_NOTPOSSIBLE;
	}

	bool skipLimit = hasBitSet(FLAG_NOLIMIT, flags);
	if (!skipLimit) {
		int32_t addCount = 0;

		if ((item->isStackable() && item->getItemCount() != count)) {
			addCount = 1;
		}

		if (findDepotChest(item->getParent()) != this) {
			if (const Container* container = item->getContainer()) {
				addCount = container->getItemHoldingCount() + 1;
			} else {
				addCount = 1;
			}
		}

		if (addCount != 0 && getItemHoldingCount() + addCount > maxDepotItems) {
			return RETURNVALUE_DEPOTISFULL;
		}
	}

	return Container::queryAdd(index, thing, count, flags, actor);
}

const DepotChest* findDepotChest(const Cylinder* cylinder)
{
	for (; cylinder; cylinder = cylinder->getParent()) {
		if (const DepotChest* chest = dynamic_cast<const DepotChest*>(cylinder)) {
			return chest;
		}
	}
	return nullptr;
}

void DepotChest::postAddNotification(Thing* thing, const Cylinder* oldParent, int32_t index, cylinderlink_t)
{
	Cylinder* parent = getParent();
	if (parent != nullptr) {
		parent->postAddNotification(thing, oldParent, index, LINK_PARENT);
	}

	save = true;
}

void DepotChest::postRemoveNotification(Thing* thing, const Cylinder* newParent, int32_t index, cylinderlink_t)
{
	Cylinder* parent = getParent();
	if (parent != nullptr) {
		parent->postRemoveNotification(thing, newParent, index, LINK_PARENT);
	}

	save = true;
}

/*Cylinder* DepotChest::getParent() const
{
    if (parent) {
        return parent->getParent();
    }
    return nullptr;
}*/
