/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "../../ride/TrackPaint.h"
#include "../../world/tile_element/TrackElement.h"
#include "../Paint.h"
#include "../support/MetalSupports.h"
#include "../tile_element/Paint.TileElement.h"
#include "../tile_element/Segment.h"
#include "../track/Segment.h"

#include <array>
#include <cstdint>

struct Ride;

namespace OpenRCT2
{
    enum class FlatTrackSupportStyle : uint8_t
    {
        none,
        metal,
        metalRotated,
    };

    // Data-driven painter for the straight Flat track piece.
    //
    // Historically every ride type carried its own ~50-line hand-written FlatTrack function that
    // differed only in: the lift-hill chain sprite quad, the plain sprite quad, the metal support
    // style/offset, and the tunnel group. Everything else (bounding box, centre support, flat
    // tunnel, kStraightFlat blocked segments, default general support height) is identical across
    // every ride that fits this shape. This template lifts those four varying things into
    // parameters; each ride contributes a one-line data row at its dispatch site instead.
    //
    // Rides whose Flat piece does something unusual (water troughs, 12-sprite rail/tie layering,
    // wooden track) keep their bespoke function — dispatch is per-ride, so the two coexist freely.
    template<
        const std::array<ImageIndex, kNumOrthogonalDirections>& kChainSprites,
        const std::array<ImageIndex, kNumOrthogonalDirections>& kSprites, const FlatTrackSupportStyle kSupportStyle,
        const int8_t kSupportOffset, const TunnelGroup kTunnelGroup>
    void trackPaintFlat(
        PaintSession& session, const Ride& ride, const uint8_t trackSequence, const Direction direction, const int32_t height,
        const OpenRCT2::TrackElement& trackElement, const SupportType supportType)
    {
        const auto& sprites = trackElement.HasChain() ? kChainSprites : kSprites;
        PaintAddImageAsParentRotated(
            session, direction, session.TrackColours.WithIndex(sprites[direction]), { 0, 0, height },
            { { 0, 6, height }, { 32, 20, 3 } });

        if (TrackPaintUtilShouldPaintSupports(session.MapPosition))
        {
            if constexpr (kSupportStyle == FlatTrackSupportStyle::metalRotated)
            {
                MetalASupportsPaintSetupRotated(
                    session, supportType.metal, MetalSupportPlace::centre, direction, kSupportOffset, height,
                    session.SupportColours);
            }
            else if constexpr (kSupportStyle == FlatTrackSupportStyle::metal)
            {
                MetalASupportsPaintSetup(
                    session, supportType.metal, MetalSupportPlace::centre, kSupportOffset, height, session.SupportColours);
            }
        }

        PaintUtilPushTunnelRotated(session, direction, height, kTunnelGroup, TunnelSubType::Flat);
        PaintUtilSetSegmentSupportHeight(
            session, PaintUtilRotateSegments(BlockedSegments::kStraightFlat, direction), 0xFFFF, 0);
        PaintUtilSetGeneralSupportHeight(session, height + kDefaultGeneralSupportHeight);
    }

    // Data-driven painter for the Flat piece of inverted/hung-track rides (inverted, lay-down,
    // suspended, impulse, ...). These share a skeleton that is distinct from the upright one above:
    // the track sprite is raised by a per-ride z-offset, the bounding box sits at its own z and
    // depth, supports use plain centre metal at height + a per-ride extra, and — importantly — the
    // segment height is set BEFORE the supports are painted (the original asm order, preserved here
    // because support placement can depend on it). Only the per-ride offsets/heights and the sprite
    // quads vary; everything else is shared.
    template<
        const std::array<ImageIndex, kNumOrthogonalDirections>& kChainSprites,
        const std::array<ImageIndex, kNumOrthogonalDirections>& kSprites, const int32_t kImageZOffset,
        const int32_t kBoundBoxZOffset, const int32_t kBoundBoxSizeZ, const int32_t kSupportHeightExtra,
        const int32_t kGeneralSupportHeightExtra, const TunnelGroup kTunnelGroup>
    void trackPaintFlatInverted(
        PaintSession& session, const Ride& ride, const uint8_t trackSequence, const Direction direction, const int32_t height,
        const OpenRCT2::TrackElement& trackElement, const SupportType supportType)
    {
        const auto& sprites = trackElement.HasChain() ? kChainSprites : kSprites;
        PaintAddImageAsParentRotated(
            session, direction, session.TrackColours.WithIndex(sprites[direction]), { 0, 0, height + kImageZOffset },
            { { 0, 6, height + kBoundBoxZOffset }, { 32, 20, kBoundBoxSizeZ } });

        PaintUtilSetSegmentSupportHeight(
            session, PaintUtilRotateSegments(BlockedSegments::kStraightFlat, direction), 0xFFFF, 0);
        if (TrackPaintUtilShouldPaintSupports(session.MapPosition))
        {
            MetalASupportsPaintSetup(
                session, supportType.metal, MetalSupportPlace::centre, 0, height + kSupportHeightExtra, session.SupportColours);
        }

        PaintUtilPushTunnelRotated(session, direction, height, kTunnelGroup, TunnelSubType::Flat);
        PaintUtilSetGeneralSupportHeight(session, height + kGeneralSupportHeightExtra);
    }

    // Data-driven painter for the straight 25-degree-up track piece of upright rides. Same idea as
    // trackPaintFlat, but the slope adds a direction-dependent tunnel (a SlopeStart below the tile
    // for directions 0/3, a SlopeEnd above it for 1/2) and a per-ride general support height. The
    // sprite quad, centre metal support and kStraightFlat segments are otherwise shared.
    template<
        const std::array<ImageIndex, kNumOrthogonalDirections>& kChainSprites,
        const std::array<ImageIndex, kNumOrthogonalDirections>& kSprites, const FlatTrackSupportStyle kSupportStyle,
        const int8_t kSupportOffset, const int32_t kGeneralSupportHeightExtra, const TunnelGroup kTunnelGroup>
    void trackPaint25DegUp(
        PaintSession& session, const Ride& ride, const uint8_t trackSequence, const Direction direction, const int32_t height,
        const OpenRCT2::TrackElement& trackElement, const SupportType supportType)
    {
        const auto& sprites = trackElement.HasChain() ? kChainSprites : kSprites;
        PaintAddImageAsParentRotated(
            session, direction, session.TrackColours.WithIndex(sprites[direction]), { 0, 0, height },
            { { 0, 6, height }, { 32, 20, 3 } });

        if (TrackPaintUtilShouldPaintSupports(session.MapPosition))
        {
            if constexpr (kSupportStyle == FlatTrackSupportStyle::metalRotated)
            {
                MetalASupportsPaintSetupRotated(
                    session, supportType.metal, MetalSupportPlace::centre, direction, kSupportOffset, height,
                    session.SupportColours);
            }
            else if constexpr (kSupportStyle == FlatTrackSupportStyle::metal)
            {
                MetalASupportsPaintSetup(
                    session, supportType.metal, MetalSupportPlace::centre, kSupportOffset, height, session.SupportColours);
            }
        }

        if (direction == 0 || direction == 3)
        {
            PaintUtilPushTunnelRotated(session, direction, height - 8, kTunnelGroup, TunnelSubType::SlopeStart);
        }
        else
        {
            PaintUtilPushTunnelRotated(session, direction, height + 8, kTunnelGroup, TunnelSubType::SlopeEnd);
        }

        PaintUtilSetSegmentSupportHeight(
            session, PaintUtilRotateSegments(BlockedSegments::kStraightFlat, direction), 0xFFFF, 0);
        PaintUtilSetGeneralSupportHeight(session, height + kGeneralSupportHeightExtra);
    }
    // Data-driven painter for the 25-degree-up piece of inverted/hung-track rides. Combines the
    // inverted skeleton (segment height set before supports, sprite raised by a z-offset) with the
    // slope's directional tunnel, plus a per-direction side support placement: the standard
    // inverted-slope arrangement topRightSide/bottomRightSide/bottomLeftSide/topLeftSide painted
    // with the non-rotated helper. Only the raised offsets and support/general heights vary.
    template<
        const std::array<ImageIndex, kNumOrthogonalDirections>& kChainSprites,
        const std::array<ImageIndex, kNumOrthogonalDirections>& kSprites, const int32_t kImageZOffset,
        const int32_t kBoundBoxZOffset, const int32_t kSupportHeightExtra, const int32_t kGeneralSupportHeightExtra,
        const TunnelGroup kTunnelGroup>
    void trackPaint25DegUpInverted(
        PaintSession& session, const Ride& ride, const uint8_t trackSequence, const Direction direction, const int32_t height,
        const OpenRCT2::TrackElement& trackElement, const SupportType supportType)
    {
        const auto& sprites = trackElement.HasChain() ? kChainSprites : kSprites;
        PaintAddImageAsParentRotated(
            session, direction, session.TrackColours.WithIndex(sprites[direction]), { 0, 0, height + kImageZOffset },
            { { 0, 6, height + kBoundBoxZOffset }, { 32, 20, 3 } });

        PaintUtilSetSegmentSupportHeight(
            session, PaintUtilRotateSegments(BlockedSegments::kStraightFlat, direction), 0xFFFF, 0);
        if (TrackPaintUtilShouldPaintSupports(session.MapPosition))
        {
            static constexpr MetalSupportPlace kPlaces[kNumOrthogonalDirections] = {
                MetalSupportPlace::topRightSide,
                MetalSupportPlace::bottomRightSide,
                MetalSupportPlace::bottomLeftSide,
                MetalSupportPlace::topLeftSide,
            };
            MetalASupportsPaintSetup(
                session, supportType.metal, kPlaces[direction], 0, height + kSupportHeightExtra, session.SupportColours);
        }

        if (direction == 0 || direction == 3)
        {
            PaintUtilPushTunnelRotated(session, direction, height - 8, kTunnelGroup, TunnelSubType::SlopeStart);
        }
        else
        {
            PaintUtilPushTunnelRotated(session, direction, height + 8, kTunnelGroup, TunnelSubType::SlopeEnd);
        }
        PaintUtilSetGeneralSupportHeight(session, height + kGeneralSupportHeightExtra);
    }
    // Data-driven painter for the flat-to-25-degree-up transition piece of upright rides. Identical
    // to trackPaint25DegUp except the transition tunnel: a flat tunnel at tile height for directions
    // 0/3 and a SlopeEnd (also at tile height, no vertical offset) for 1/2.
    template<
        const std::array<ImageIndex, kNumOrthogonalDirections>& kChainSprites,
        const std::array<ImageIndex, kNumOrthogonalDirections>& kSprites, const FlatTrackSupportStyle kSupportStyle,
        const int8_t kSupportOffset, const int32_t kGeneralSupportHeightExtra, const TunnelGroup kTunnelGroup>
    void trackPaintFlatTo25DegUp(
        PaintSession& session, const Ride& ride, const uint8_t trackSequence, const Direction direction, const int32_t height,
        const OpenRCT2::TrackElement& trackElement, const SupportType supportType)
    {
        const auto& sprites = trackElement.HasChain() ? kChainSprites : kSprites;
        PaintAddImageAsParentRotated(
            session, direction, session.TrackColours.WithIndex(sprites[direction]), { 0, 0, height },
            { { 0, 6, height }, { 32, 20, 3 } });

        if (TrackPaintUtilShouldPaintSupports(session.MapPosition))
        {
            if constexpr (kSupportStyle == FlatTrackSupportStyle::metalRotated)
            {
                MetalASupportsPaintSetupRotated(
                    session, supportType.metal, MetalSupportPlace::centre, direction, kSupportOffset, height,
                    session.SupportColours);
            }
            else if constexpr (kSupportStyle == FlatTrackSupportStyle::metal)
            {
                MetalASupportsPaintSetup(
                    session, supportType.metal, MetalSupportPlace::centre, kSupportOffset, height, session.SupportColours);
            }
        }

        if (direction == 0 || direction == 3)
        {
            PaintUtilPushTunnelRotated(session, direction, height, kTunnelGroup, TunnelSubType::Flat);
        }
        else
        {
            PaintUtilPushTunnelRotated(session, direction, height, kTunnelGroup, TunnelSubType::SlopeEnd);
        }

        PaintUtilSetSegmentSupportHeight(
            session, PaintUtilRotateSegments(BlockedSegments::kStraightFlat, direction), 0xFFFF, 0);
        PaintUtilSetGeneralSupportHeight(session, height + kGeneralSupportHeightExtra);
    }

    // Data-driven painter for the 25-degree-up-to-flat transition piece of upright rides. Identical
    // to trackPaintFlatTo25DegUp except the transition tunnel: a flat tunnel below the tile (height-8)
    // for directions 0/3 and a FlatTo25Deg tunnel above it (height+8) for 1/2.
    template<
        const std::array<ImageIndex, kNumOrthogonalDirections>& kChainSprites,
        const std::array<ImageIndex, kNumOrthogonalDirections>& kSprites, const FlatTrackSupportStyle kSupportStyle,
        const int8_t kSupportOffset, const int32_t kGeneralSupportHeightExtra, const TunnelGroup kTunnelGroup>
    void trackPaint25DegUpToFlat(
        PaintSession& session, const Ride& ride, const uint8_t trackSequence, const Direction direction, const int32_t height,
        const OpenRCT2::TrackElement& trackElement, const SupportType supportType)
    {
        const auto& sprites = trackElement.HasChain() ? kChainSprites : kSprites;
        PaintAddImageAsParentRotated(
            session, direction, session.TrackColours.WithIndex(sprites[direction]), { 0, 0, height },
            { { 0, 6, height }, { 32, 20, 3 } });

        if (TrackPaintUtilShouldPaintSupports(session.MapPosition))
        {
            if constexpr (kSupportStyle == FlatTrackSupportStyle::metalRotated)
            {
                MetalASupportsPaintSetupRotated(
                    session, supportType.metal, MetalSupportPlace::centre, direction, kSupportOffset, height,
                    session.SupportColours);
            }
            else if constexpr (kSupportStyle == FlatTrackSupportStyle::metal)
            {
                MetalASupportsPaintSetup(
                    session, supportType.metal, MetalSupportPlace::centre, kSupportOffset, height, session.SupportColours);
            }
        }

        if (direction == 0 || direction == 3)
        {
            PaintUtilPushTunnelRotated(session, direction, height - 8, kTunnelGroup, TunnelSubType::Flat);
        }
        else
        {
            PaintUtilPushTunnelRotated(session, direction, height + 8, kTunnelGroup, TunnelSubType::FlatTo25Deg);
        }

        PaintUtilSetSegmentSupportHeight(
            session, PaintUtilRotateSegments(BlockedSegments::kStraightFlat, direction), 0xFFFF, 0);
        PaintUtilSetGeneralSupportHeight(session, height + kGeneralSupportHeightExtra);
    }

    // Data-driven painter for the flat-to-25-degree-up transition of inverted/hung-track rides.
    // Identical to trackPaint25DegUpInverted (raised sprite, segment-before-supports, the per-direction
    // side support place switch painted with the non-rotated helper) except the transition tunnel: a
    // flat tunnel at tile height for directions 0/3 and a SlopeEnd (also at tile height) for 1/2.
    template<
        const std::array<ImageIndex, kNumOrthogonalDirections>& kChainSprites,
        const std::array<ImageIndex, kNumOrthogonalDirections>& kSprites, const int32_t kImageZOffset,
        const int32_t kBoundBoxZOffset, const int32_t kSupportHeightExtra, const int32_t kGeneralSupportHeightExtra,
        const TunnelGroup kTunnelGroup>
    void trackPaintFlatTo25DegUpInverted(
        PaintSession& session, const Ride& ride, const uint8_t trackSequence, const Direction direction, const int32_t height,
        const OpenRCT2::TrackElement& trackElement, const SupportType supportType)
    {
        const auto& sprites = trackElement.HasChain() ? kChainSprites : kSprites;
        PaintAddImageAsParentRotated(
            session, direction, session.TrackColours.WithIndex(sprites[direction]), { 0, 0, height + kImageZOffset },
            { { 0, 6, height + kBoundBoxZOffset }, { 32, 20, 3 } });

        PaintUtilSetSegmentSupportHeight(
            session, PaintUtilRotateSegments(BlockedSegments::kStraightFlat, direction), 0xFFFF, 0);
        if (TrackPaintUtilShouldPaintSupports(session.MapPosition))
        {
            static constexpr MetalSupportPlace kPlaces[kNumOrthogonalDirections] = {
                MetalSupportPlace::topRightSide,
                MetalSupportPlace::bottomRightSide,
                MetalSupportPlace::bottomLeftSide,
                MetalSupportPlace::topLeftSide,
            };
            MetalASupportsPaintSetup(
                session, supportType.metal, kPlaces[direction], 0, height + kSupportHeightExtra, session.SupportColours);
        }

        if (direction == 0 || direction == 3)
        {
            PaintUtilPushTunnelRotated(session, direction, height, kTunnelGroup, TunnelSubType::Flat);
        }
        else
        {
            PaintUtilPushTunnelRotated(session, direction, height, kTunnelGroup, TunnelSubType::SlopeEnd);
        }
        PaintUtilSetGeneralSupportHeight(session, height + kGeneralSupportHeightExtra);
    }

    // Data-driven painter for the 25-degree-up-to-flat transition of inverted/hung-track rides. Same
    // skeleton as trackPaintFlatTo25DegUpInverted, but the transition tunnel is a flat tunnel below the
    // tile (height-8) for directions 0/3 and a FlatTo25Deg tunnel above it (height+8) for 1/2.
    template<
        const std::array<ImageIndex, kNumOrthogonalDirections>& kChainSprites,
        const std::array<ImageIndex, kNumOrthogonalDirections>& kSprites, const int32_t kImageZOffset,
        const int32_t kBoundBoxZOffset, const int32_t kSupportHeightExtra, const int32_t kGeneralSupportHeightExtra,
        const TunnelGroup kTunnelGroup>
    void trackPaint25DegUpToFlatInverted(
        PaintSession& session, const Ride& ride, const uint8_t trackSequence, const Direction direction, const int32_t height,
        const OpenRCT2::TrackElement& trackElement, const SupportType supportType)
    {
        const auto& sprites = trackElement.HasChain() ? kChainSprites : kSprites;
        PaintAddImageAsParentRotated(
            session, direction, session.TrackColours.WithIndex(sprites[direction]), { 0, 0, height + kImageZOffset },
            { { 0, 6, height + kBoundBoxZOffset }, { 32, 20, 3 } });

        PaintUtilSetSegmentSupportHeight(
            session, PaintUtilRotateSegments(BlockedSegments::kStraightFlat, direction), 0xFFFF, 0);
        if (TrackPaintUtilShouldPaintSupports(session.MapPosition))
        {
            static constexpr MetalSupportPlace kPlaces[kNumOrthogonalDirections] = {
                MetalSupportPlace::topRightSide,
                MetalSupportPlace::bottomRightSide,
                MetalSupportPlace::bottomLeftSide,
                MetalSupportPlace::topLeftSide,
            };
            MetalASupportsPaintSetup(
                session, supportType.metal, kPlaces[direction], 0, height + kSupportHeightExtra, session.SupportColours);
        }

        if (direction == 0 || direction == 3)
        {
            PaintUtilPushTunnelRotated(session, direction, height - 8, kTunnelGroup, TunnelSubType::Flat);
        }
        else
        {
            PaintUtilPushTunnelRotated(session, direction, height + 8, kTunnelGroup, TunnelSubType::FlatTo25Deg);
        }
        PaintUtilSetGeneralSupportHeight(session, height + kGeneralSupportHeightExtra);
    }
} // namespace OpenRCT2
