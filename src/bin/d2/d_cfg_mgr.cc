// Copyright (C) 2013 Internet Systems Consortium, Inc. ("ISC")
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

#include <config/ccsession.h>
#include <d2/d2_log.h>
#include <dhcp/libdhcp++.h>
#include <d2/d_cfg_mgr.h>
#include <dhcpsrv/dhcp_parsers.h>
#include <util/encode/hex.h>
#include <util/strutil.h>

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include <limits>
#include <iostream>
#include <vector>
#include <map>

using namespace std;
using namespace bundy;
using namespace bundy::dhcp;
using namespace bundy::data;
using namespace bundy::asiolink;

namespace bundy {
namespace d2 {

// *********************** DCfgContextBase  *************************

DCfgContextBase::DCfgContextBase():
        boolean_values_(new BooleanStorage()),
        uint32_values_(new Uint32Storage()),
        string_values_(new StringStorage()) {
    }

DCfgContextBase::DCfgContextBase(const DCfgContextBase& rhs):
        boolean_values_(new BooleanStorage(*(rhs.boolean_values_))),
        uint32_values_(new Uint32Storage(*(rhs.uint32_values_))),
        string_values_(new StringStorage(*(rhs.string_values_))) {
}

void
DCfgContextBase::getParam(const std::string& name, bool& value, bool optional) {
    try {
        value = boolean_values_->getParam(name);
    } catch (DhcpConfigError& ex) {
        // If the parameter is not optional, re-throw the exception.
        if (!optional) {
            throw;
        }
    }
}


void
DCfgContextBase::getParam(const std::string& name, uint32_t& value,
                          bool optional) {
    try {
        value = uint32_values_->getParam(name);
    } catch (DhcpConfigError& ex) {
        // If the parameter is not optional, re-throw the exception.
        if (!optional) {
            throw;
        }
    }
}

void
DCfgContextBase::getParam(const std::string& name, std::string& value,
                          bool optional) {
    try {
        value = string_values_->getParam(name);
    } catch (DhcpConfigError& ex) {
        // If the parameter is not optional, re-throw the exception.
        if (!optional) {
            throw;
        }
    }
}

DCfgContextBase::~DCfgContextBase() {
}

// *********************** DCfgMgrBase  *************************

DCfgMgrBase::DCfgMgrBase(DCfgContextBasePtr context)
    : parse_order_(), context_(context) {
    if (!context_) {
        bundy_throw(DCfgMgrBaseError, "DCfgMgrBase ctor: context cannot be NULL");
    }
}

DCfgMgrBase::~DCfgMgrBase() {
}

bundy::data::ConstElementPtr
DCfgMgrBase::parseConfig(bundy::data::ConstElementPtr config_set) {
    LOG_DEBUG(dctl_logger, DBGLVL_COMMAND,
                DCTL_CONFIG_START).arg(config_set->str());

    if (!config_set) {
        return (bundy::config::createAnswer(1,
                                    std::string("Can't parse NULL config")));
    }

    // The parsers implement data inheritance by directly accessing
    // configuration context. For this reason the data parsers must store
    // the parsed data into context immediately. This may cause data
    // inconsistency if the parsing operation fails after the context has been
    // modified. We need to preserve the original context here
    // so as we can rollback changes when an error occurs.
    DCfgContextBasePtr original_context = context_->clone();

    // Answer will hold the result returned to the caller.
    ConstElementPtr answer;

    // Holds the name of the element being parsed.
    std::string element_id;

    try {
        // Grab a map of element_ids and their data values from the new
        // configuration set.
        const std::map<std::string, ConstElementPtr>& values_map =
                                                        config_set->mapValue();

        // Use a pre-ordered list of element ids to parse the elements in a
        // specific order if the list (parser_order_) is not empty; otherwise
        // elements are parsed in the order the value_map presents them.

        if (!parse_order_.empty()) {
            // For each element_id in the parse order list, look for it in the
            // value map.  If the element exists in the map, pass it and it's
            // associated data in for parsing.
            // If there is no matching entry in the value map an error is
            // thrown.  Note, that elements tagged as "optional" from the user
            // perspective must still have default or empty entries in the
            // configuration set to be parsed.
            int parsed_count = 0;
            std::map<std::string, ConstElementPtr>::const_iterator it;
            BOOST_FOREACH(element_id, parse_order_) {
                it = values_map.find(element_id);
                if (it != values_map.end()) {
                    ++parsed_count;
                    buildAndCommit(element_id, it->second);
                }
                else {
                    LOG_ERROR(dctl_logger, DCTL_ORDER_NO_ELEMENT)
                              .arg(element_id);
                    bundy_throw(DCfgMgrBaseError, "Element:" << element_id <<
                              " is listed in the parse order but is not "
                              " present in the configuration");
                }
            }

            // NOTE: When using ordered parsing, the parse order list MUST
            // include every possible element id that the value_map may contain.
            // Entries in the map that are not in the parse order, would not be
            // parsed. For now we will flag this as a programmatic error.  One
            // could attempt to adjust for this, by identifying such entries
            // and parsing them either first or last but which would be correct?
            // Better to hold the engineer accountable.  So, if we parsed none
            // or we parsed fewer than are in the map; then either the parse i
            // order is incomplete OR the map has unsupported values.
            if (!parsed_count ||
                (parsed_count && ((parsed_count + 1) < values_map.size()))) {
                LOG_ERROR(dctl_logger, DCTL_ORDER_ERROR);
                bundy_throw(DCfgMgrBaseError,
                        "Configuration contains elements not in parse order");
            }
        } else {
            // Order doesn't matter so iterate over the value map directly.
            // Pass each element and it's associated data in to be parsed.
            ConfigPair config_pair;
            BOOST_FOREACH(config_pair, values_map) {
                element_id = config_pair.first;
                buildAndCommit(element_id, config_pair.second);
            }
        }

        // Everything was fine. Configuration set processed successfully.
        LOG_INFO(dctl_logger, DCTL_CONFIG_COMPLETE).arg("");
        answer = bundy::config::createAnswer(0, "Configuration committed.");

    } catch (const bundy::Exception& ex) {
        LOG_ERROR(dctl_logger, DCTL_PARSER_FAIL).arg(element_id).arg(ex.what());
        answer = bundy::config::createAnswer(1,
                     string("Configuration parsing failed: ") + ex.what());

        // An error occurred, so make sure that we restore original context.
        context_ = original_context;
        return (answer);
    }

    return (answer);
}

void DCfgMgrBase::buildAndCommit(std::string& element_id,
                                 bundy::data::ConstElementPtr value) {
    // Call derivation's implementation to create the appropriate parser
    // based on the element id.
    ParserPtr parser = createConfigParser(element_id);
    if (!parser) {
        bundy_throw(DCfgMgrBaseError, "Could not create parser");
    }

    try {
        // Invoke the parser's build method passing in the value. This will
        // "convert" the Element form of value into the actual data item(s)
        // and store them in parser's local storage.
        parser->build(value);

        // Invoke the parser's commit method. This "writes" the the data
        // item(s) stored locally by the parser into the context.  (Note that
        // parsers are free to do more than update the context, but that is an
        // nothing something we are concerned with here.)
        parser->commit();
    } catch (const bundy::Exception& ex) {
        bundy_throw(DCfgMgrBaseError,
                  "Could not build and commit: " << ex.what());
    } catch (...) {
        bundy_throw(DCfgMgrBaseError, "Non-ISC exception occurred");
    }
}

}; // end of bundy::dhcp namespace
}; // end of bundy namespace

