/*
 * Segment.cpp
 *****************************************************************************
 * Copyright (C) 2010 - 2011 Klagenfurt University
 *
 * Created on: Aug 10, 2010
 * Authors: Christopher Mueller <christopher.mueller@itec.uni-klu.ac.at>
 *          Christian Timmerer  <christian.timmerer@itec.uni-klu.ac.at>
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

#include "DASHSegment.h"
#include "../adaptative/playlist/BaseRepresentation.h"
#include "../mp4/IndexReader.hpp"
#include "../adaptative/playlist/AbstractPlaylist.hpp"
#include "../adaptative/playlist/SegmentChunk.hpp"

using namespace adaptative::playlist;
using namespace dash::mpd;
using namespace dash::mp4;

DashIndexSegment::DashIndexSegment(ICanonicalUrl *parent) :
    IndexSegment(parent)
{
}

void DashIndexSegment::onChunkDownload(block_t **pp_block, SegmentChunk *, BaseRepresentation *rep)
{
    if(!rep || ((*pp_block)->i_flags & BLOCK_FLAG_HEADER) == 0 )
        return;

    IndexReader br(rep->getPlaylist()->getVLCObject());
    br.parseIndex(*pp_block, rep);
}