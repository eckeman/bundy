// Copyright (C) 2012  Internet Systems Consortium, Inc. ("ISC")
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
// REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
// AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
// LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
// OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#ifndef DATASRC_MEMORY_ZONE_FINDER_H
#define DATASRC_MEMORY_ZONE_FINDER_H 1

#include <datasrc/memory/zone_data.h>
#include <datasrc/memory/treenode_rrset.h>

#include <datasrc/zone_finder.h>
#include <dns/name.h>
#include <dns/rrset.h>
#include <dns/rrtype.h>

#include <string>

namespace bundy {
namespace datasrc {
namespace memory {
namespace internal {
// intermediate result context, only used in the zone finder implementation.
class ZoneFinderResultContext;
}

/// A derived zone finder class intended to be used with the memory data
/// source, using ZoneData for its contents.
class InMemoryZoneFinder : boost::noncopyable, public ZoneFinder {
public:
    /// \brief Constructor.
    ///
    /// Since ZoneData does not keep RRClass information, but this
    /// information is needed in order to construct actual RRsets,
    /// this needs to be passed here (the datasource client should
    /// have this information). In the future, this may be replaced
    /// by some construction to pull TreeNodeRRsets from a pool, but
    /// currently, these are created dynamically with the given RRclass
    ///
    /// \param zone_data The ZoneData containing the zone.
    /// \param rrclass The RR class of the zone
    InMemoryZoneFinder(const ZoneData& zone_data,
                       const bundy::dns::RRClass& rrclass) :
        zone_data_(zone_data),
        rrclass_(rrclass)
    {}

    /// \brief Find an RRset in the datasource
    virtual boost::shared_ptr<ZoneFinder::Context> find(
        const bundy::dns::Name& name,
        const bundy::dns::RRType& type,
        const FindOptions options = FIND_DEFAULT);

    /// \brief Search for an RRset of given RR type at the zone origin
    /// specialized for in-memory data source.
    ///
    /// This specialized version exploits internal data structure to find
    /// RRsets at the zone origin and (if \c use_minttl is true) extract
    /// the SOA Minimum TTL much more efficiently.
    virtual boost::shared_ptr<ZoneFinder::Context> findAtOrigin(
        const bundy::dns::RRType& type, bool use_minttl,
        FindOptions options);

    /// \brief Version of find that returns all types at once
    ///
    /// It acts the same as find, just that when the correct node is found,
    /// all the RRsets are filled into the target parameter instead of being
    /// returned by the result.
    virtual boost::shared_ptr<ZoneFinder::Context> findAll(
        const bundy::dns::Name& name,
        std::vector<bundy::dns::ConstRRsetPtr>& target,
        const FindOptions options = FIND_DEFAULT);

    /// Look for NSEC3 for proving (non)existence of given name.
    ///
    /// See documentation in \c Zone.
    virtual FindNSEC3Result
    findNSEC3(const bundy::dns::Name& name, bool recursive);

    /// \brief Returns the origin of the zone.
    virtual bundy::dns::Name getOrigin() const;

    /// \brief Returns the RR class of the zone.
    virtual bundy::dns::RRClass getClass() const {
        return (rrclass_);
    }

private:
    /// \brief In-memory version of finder context.
    ///
    /// The implementation (and any specialized interface) is completely local
    /// to the InMemoryZoneFinder class, so it's defined as private
    class Context;

    /// Actual implementation for both find() and findAll()
    internal::ZoneFinderResultContext findInternal(
        const bundy::dns::Name& name,
        const bundy::dns::RRType& type,
        std::vector<bundy::dns::ConstRRsetPtr>* target,
        const FindOptions options =
        FIND_DEFAULT);

    const ZoneData& zone_data_;
    const bundy::dns::RRClass rrclass_;
};

} // namespace memory
} // namespace datasrc
} // namespace bundy

#endif // DATASRC_MEMORY_ZONE_FINDER_H

// Local Variables:
// mode: c++
// End:
