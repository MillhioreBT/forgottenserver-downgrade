# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased]

### Revert
- Changes that allowed obtaining a creature's master in the onDeath event have been reverted. For now, it will remain this way to eliminate the possibility of an unexpected crash. This was reported [here](https://github.com/otland/forgottenserver/issues/4786) and fixed here with a [revert](https://github.com/otland/forgottenserver/pull/4803).
I will be planning how to restore this feature later.

### Breaking Changes
```lua
local e = CreatureEvent("ondeath")

function e.onDeath(creature, corpse, killer, mostDamageKiller, lastHitUnjustified, mostDamageUnjustified)
    if killer:isPlayer() then
        local master = creature:getMaster() -- This line will no longer work
        if master then
            master:sendTextMessage(MESSAGE_INFO_DESCR, "You have slain " .. creature:getName() .. ".")
        end
    end
    return true
end

e:register()
```
