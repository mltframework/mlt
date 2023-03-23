/**
 * MltChain.h - Chain wrapper
 * Copyright (C) 2020-2022 Meltytech, LLC
  *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef MLTPP_CHAIN_H
#define MLTPP_CHAIN_H

#include "MltConfig.h"

#include <framework/mlt.h>

#include "MltLink.h"
#include "MltProducer.h"
#include "MltProfile.h"

namespace Mlt {
class MLTPP_DECLSPEC Chain : public Producer
{
private:
    mlt_chain instance;

public:
    Chain();
    Chain(Profile &profile, const char *id, const char *service = NULL);
    Chain(Mlt::Profile &profile);
    Chain(mlt_chain chain);
    Chain(Chain &chain);
    Chain(Chain *chain);
    Chain(Service &chain);
    virtual ~Chain();
    virtual mlt_chain get_chain();
    mlt_producer get_producer() override;
    void set_source(Mlt::Producer &source);
    Mlt::Producer get_source();
    int attach(Mlt::Link &link);
    int detach(Mlt::Link &link);
    int link_count() const;
    bool move_link(int from, int to);
    Mlt::Link *link(int index);
    void attach_normalizers();
};
} // namespace Mlt

#endif
