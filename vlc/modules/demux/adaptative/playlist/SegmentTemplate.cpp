/*****************************************************************************
 * SegmentTemplate.cpp: Implement the UrlTemplate element.
 *****************************************************************************
 * Copyright (C) 1998-2007 VLC authors and VideoLAN
 * $Id$
 *
 * Authors: Hugo Beauzée-Luyssen <beauze.h@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
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

#include "SegmentTemplate.h"
#include "SegmentTimeline.h"
#include "SegmentInformation.hpp"

using namespace adaptative::playlist;

BaseSegmentTemplate::BaseSegmentTemplate( ICanonicalUrl *parent ) :
    Segment( parent )
{
}


MediaSegmentTemplate::MediaSegmentTemplate( SegmentInformation *parent ) :
    BaseSegmentTemplate( parent ), Timelineable(), TimescaleAble( parent )
{
    debugName = "SegmentTemplate";
    classId = Segment::CLASSID_SEGMENT;
    startNumber.Set( 1 );
    initialisationSegment.Set( NULL );
    templated = true;
}

void MediaSegmentTemplate::mergeWith(MediaSegmentTemplate *updated, mtime_t prunebarrier)
{
    SegmentTimeline *timeline = segmentTimeline.Get();
    if(timeline && updated->segmentTimeline.Get())
    {
        timeline->mergeWith(*updated->segmentTimeline.Get());
        if(prunebarrier)
        {
            const uint64_t timescale = timeline->inheritTimescale();
            const uint64_t number =
                    timeline->getElementNumberByScaledPlaybackTime(prunebarrier * timescale / CLOCK_FREQ);
            timeline->pruneBySequenceNumber(number);
        }
    }
}

void MediaSegmentTemplate::pruneByPlaybackTime(mtime_t time)
{
    if(segmentTimeline.Get())
        return segmentTimeline.Get()->pruneByPlaybackTime(time);
}

size_t MediaSegmentTemplate::pruneBySequenceNumber(uint64_t number)
{
    if(segmentTimeline.Get())
        return segmentTimeline.Get()->pruneBySequenceNumber(number);
    return 0;
}

uint64_t MediaSegmentTemplate::getSequenceNumber() const
{
    return startNumber.Get();
}

void MediaSegmentTemplate::setSourceUrl(const std::string &url)
{
    sourceUrl = Url(Url::Component(url, this));
}

void MediaSegmentTemplate::debug(vlc_object_t *obj, int indent) const
{
    Segment::debug(obj, indent);
    if(segmentTimeline.Get())
        segmentTimeline.Get()->debug(obj, indent + 1);
}

InitSegmentTemplate::InitSegmentTemplate( ICanonicalUrl *parent ) :
    BaseSegmentTemplate(parent)
{
    debugName = "InitSegmentTemplate";
    classId = InitSegment::CLASSID_INITSEGMENT;
}
