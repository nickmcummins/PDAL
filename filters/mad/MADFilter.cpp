/******************************************************************************
* Copyright (c) 2016, Bradley J Chambers (brad.chambers@gmail.com)
*
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following
* conditions are met:
*
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in
*       the documentation and/or other materials provided
*       with the distribution.
*     * Neither the name of Hobu, Inc. or Flaxen Geo Consulting nor the
*       names of its contributors may be used to endorse or promote
*       products derived from this software without specific prior
*       written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
* FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
* COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
* BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
* OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
* AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
* OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
* OF SUCH DAMAGE.
****************************************************************************/

#include "MADFilter.hpp"

#include <pdal/pdal_macros.hpp>

#include <string>
#include <vector>

namespace pdal
{

static PluginInfo const s_info =
    PluginInfo("filters.mad", "Median Absolute Deviation Filter",
               "http://pdal.io/stages/filters.mad.html");

CREATE_STATIC_PLUGIN(1, 0, MADFilter, Filter, s_info)

std::string MADFilter::getName() const
{
    return s_info.name;
}

void MADFilter::addArgs(ProgramArgs& args)
{
    args.add("k", "Number of deviations", m_multiplier, 2.0);
    args.add("dimension", "Dimension on which to calculate statistics",
        m_dimName);
}

void MADFilter::prepared(PointTableRef table)
{
    PointLayoutPtr layout(table.layout());
    m_dimId = layout->findDim(m_dimName);
    if (m_dimId == Dimension::Id::Unknown)
    {
        std::ostringstream oss;
        oss << "Invalid dimension name in filters.mad 'dimension' "
            "option: '" << m_dimName << "'.";
        throw pdal_error(oss.str());
    }
}

PointViewSet MADFilter::run(PointViewPtr view)
{
    using namespace Dimension;

    PointViewSet viewSet;
    PointViewPtr output = view->makeNew();

    auto median = [](std::vector<double> vals)
    {
        std::nth_element(vals.begin(), vals.begin()+vals.size()/2, vals.end());

        return *(vals.begin()+vals.size()/2);
    };

    std::vector<double> z(view->size());
    for (PointId j = 0; j < view->size(); ++j)
        z[j] = view->getFieldAs<double>(m_dimId, j);

    double m = median(z);
    log()->get(LogLevel::Debug) << "Median value: " << m << std::endl;

    std::vector<double> a(view->size());
    for (PointId j = 0; j < view->size(); ++j)
        a[j] = std::fabs(view->getFieldAs<double>(Id::Z, j)-m);

    double mad = median(a)*1.4862;
    log()->get(LogLevel::Debug) << "MAD: " << mad << std::endl;

    for (PointId j = 0; j < view->size(); ++j)
    {
        if (a[j]/mad < m_multiplier)
            output->appendPoint(*view, j);
    }
    
    double low_fence = m - m_multiplier * mad;
    double hi_fence = m + m_multiplier * mad;
    
    log()->get(LogLevel::Debug) << "Cropping " << m_dimName
                                << " in the range (" << low_fence
                                << "," << hi_fence << ")" << std::endl;

    viewSet.erase(view);
    viewSet.insert(output);

    return viewSet;
}

} // namespace pdal
