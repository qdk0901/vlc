/*
 * Representation.cpp
 *****************************************************************************
 * Copyright © 2015 - VideoLAN and VLC Authors
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <cstdlib>

#include "Representation.hpp"
#include "M3U8.hpp"
#include "Parser.hpp"
#include "HLSSegment.hpp"
#include "../adaptative/playlist/BasePeriod.h"
#include "../adaptative/playlist/BaseAdaptationSet.h"
#include "../adaptative/playlist/SegmentList.h"

#include <ctime>

using namespace hls;
using namespace hls::playlist;

Representation::Representation  ( BaseAdaptationSet *set ) :
                BaseRepresentation( set )
{
    b_live = true;
    b_loaded = false;
    switchpolicy = SegmentInformation::SWITCH_SEGMENT_ALIGNED; /* FIXME: based on streamformat */
    nextUpdateTime = 0;
    targetDuration = 0;
    streamFormat = StreamFormat::UNKNOWN;
}

Representation::~Representation ()
{
}

StreamFormat Representation::getStreamFormat() const
{
    return streamFormat;
}

bool Representation::isLive() const
{
    return b_live;
}

bool Representation::initialized() const
{
    return b_loaded;
}

void Representation::setPlaylistUrl(const std::string &uri)
{
    playlistUrl = Url(uri);
}

Url Representation::getPlaylistUrl() const
{
    if(playlistUrl.hasScheme())
    {
        return playlistUrl;
    }
    else
    {
        Url ret = getParentUrlSegment();
        if(!playlistUrl.empty())
            ret.append(playlistUrl);
        return ret;
    }
}

void Representation::debug(vlc_object_t *obj, int indent) const
{
    BaseRepresentation::debug(obj, indent);
    if(!b_loaded)
    {
        std::string text(indent + 1, ' ');
        text.append(" (not loaded) ");
        text.append(getStreamFormat().str());
        msg_Dbg(obj, "%s", text.c_str());
    }
}

void Representation::scheduleNextUpdate(uint64_t number)
{
    const AbstractPlaylist *playlist = getPlaylist();
    const time_t now = time(NULL);

    /* Compute new update time */
    mtime_t minbuffer = getMinAheadTime(number);

    /* Update frequency must always be at least targetDuration (if any)*/
    if(targetDuration)
    {
        if(minbuffer >= 3 * CLOCK_FREQ * targetDuration)
            minbuffer -= 2 * CLOCK_FREQ * targetDuration;
    }
    else
    {
        minbuffer /= 2;
        if(targetDuration > minbuffer / CLOCK_FREQ)
            minbuffer = targetDuration * CLOCK_FREQ;
    }

    if(minbuffer < CLOCK_FREQ)
        minbuffer = CLOCK_FREQ;

    nextUpdateTime = now + minbuffer / CLOCK_FREQ;

    msg_Dbg(playlist->getVLCObject(), "Updated playlist ID %s, next update in %" PRId64 "s",
            getID().str().c_str(), (mtime_t) nextUpdateTime - now);

    debug(playlist->getVLCObject(), 0);
}

bool Representation::needsUpdate() const
{
    return !b_loaded || (isLive() && nextUpdateTime < time(NULL));
}

bool Representation::runLocalUpdates(mtime_t, uint64_t number, bool prune)
{
    const time_t now = time(NULL);
    const AbstractPlaylist *playlist = getPlaylist();
    if(!b_loaded || (isLive() && nextUpdateTime < now))
    {
        M3U8Parser parser;
        parser.appendSegmentsFromPlaylistURI(playlist->getVLCObject(), this);
        b_loaded = true;

        if(prune)
            pruneBySegmentNumber(number);

        return true;
    }

    return true;
}

uint64_t Representation::translateSegmentNumber(uint64_t num, const SegmentInformation *from) const
{
    if(consistentSegmentNumber())
        return num;
    ISegment *fromSeg = from->getSegment(SegmentInfoType::INFOTYPE_MEDIA, num);
    HLSSegment *fromHlsSeg = dynamic_cast<HLSSegment *>(fromSeg);
    if(!fromHlsSeg)
        return 1;
    const mtime_t utcTime = fromHlsSeg->getUTCTime();

    std::vector<ISegment *> list;
    std::vector<ISegment *>::const_iterator it;
    getSegments(SegmentInfoType::INFOTYPE_MEDIA, list);
    for(it=list.begin(); it != list.end(); ++it)
    {
        const HLSSegment *hlsSeg = dynamic_cast<HLSSegment *>(*it);
        if(hlsSeg)
        {
            if (hlsSeg->getUTCTime() <= utcTime || it == list.begin())
                num = hlsSeg->getSequenceNumber();
            else
                return num;
        }
    }

    return 1;
}