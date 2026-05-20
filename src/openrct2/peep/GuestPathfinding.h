/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "../ride/RideTypes.h"
#include "../world/Location.hpp"

#include <memory>

namespace OpenRCT2
{
    struct Guest;
    struct Peep;
    struct TileElement;
} // namespace OpenRCT2

namespace OpenRCT2::PathFinding
{
    // When guests walk along paths wider than the pathfinder's natural corridor, this lets them
    // occasionally drift sideways into connected adjacent lanes (preferring emptier ones) so the
    // full width of the path is used. It affects the deterministic simulation, so on a network
    // client the server's value (below) is used instead of the local config option.
    extern bool gSpreadGuestsOnWidePathsInNetworkPlay;
    bool ShouldSpreadGuestsOnWidePaths();

    Direction ChooseDirection(
        const TileCoordsXYZ& loc, const TileCoordsXYZ& goal, Peep& peep, bool ignoreForeignQueues, RideId queueRideIndex);

    int32_t CalculateNextDestination(Guest& peep);

    int32_t GuestPathFindParkEntranceEntering(Peep& peep, uint8_t edges);

    int32_t GuestPathFindPeepSpawn(Peep& peep, uint8_t edges);

    int32_t GuestPathFindParkEntranceLeaving(Peep& peep, uint8_t edges);

    /**
     * Invalidates cached A* routes by bumping the path-layout generation. Called
     * whenever the tile layout changes so cached routes are recomputed against the
     * current map. Cheap (an integer increment); only relevant when A* pathfinding is
     * enabled, harmless otherwise.
     */
    void NotifyPathLayoutChanged();

} // namespace OpenRCT2::PathFinding
