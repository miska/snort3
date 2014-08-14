/*
** Copyright (C) 2014 Cisco and/or its affiliates. All rights reserved.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License Version 2 as
** published by the Free Software Foundation.  You may not use, modify or
** distribute this program under any other version of the GNU General
** Public License.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

// ip.h author Josh Rosenbaum <jrosenba@cisco.com>

#ifndef PROTOCOLS_IP_H
#define PROTOCOLS_IP_H

#ifndef WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#else /* !WIN32 */
#include <netinet/in_systm.h>
#ifndef IFNAMSIZ
#define IFNAMESIZ MAX_ADAPTER_NAME
#endif /* !IFNAMSIZ */
#endif /* !WIN32 */

#include <cstring>

#include "protocols/ipv4.h"
#include "protocols/ipv6.h"
#include "sfip/sfip_t.h"


struct Packet;

// FIXIT : can I assume api si always valid?  i.e. if not ip4, then ipv6?
//          or if not ip4, also make sure its not ip6

namespace ip
{

// keeping this as a class to avoid confusion.
class IpApi
{
public:
//    IpApi();   constructor and destructor MUST remain a trivial. Adding
//    ~IpApi();  any non-trivial code will cause a compilation failure.

    void set(const IPHdr* h4);
    void set(const ipv6::IP6RawHdr* h6);
    bool set(const uint8_t* raw_ip_data);
    void reset();
    const sfip_t *get_src();
    const sfip_t *get_dst();
    uint32_t id(const Packet* const p) const;
    uint16_t off(const Packet* const p) const;
    // returns a pointer to this ip layer's data
    const uint8_t* ip_data() const;
    // returns the length of the ip header + length in host byte order
    uint16_t dgram_len() const;
    // returns this ip layer's payload length in host byte order
    uint16_t pay_len() const;
    // return the ip_len field in host byte order
    uint16_t actual_ip_len() const;
    // overloaded == operators.
    friend bool operator==(const IpApi& lhs, const IpApi& rhs);
    friend bool operator!=(const IpApi& lhs, const IpApi& rhs);


    // returns true if this api is set.
    inline bool is_valid() const
    { return (ip4h || ip6h); }

    inline bool is_ip6() const
    { return ip6h; }

    inline bool is_ip4() const
    { return ip4h; }

    inline const IPHdr* get_ip4h() const
    { return ip4h; }

    inline const ipv6::IP6RawHdr* get_ip6h() const
    { return ip6h; }

    inline uint16_t tos() const
    { return ip4h ? ip4h->get_tos() : ip6h ? ip6h->get_tos() : 0; }

    inline uint8_t ttl() const
    { return ip4h ? ip4h->get_ttl() : ip6h ? ip6h->get_hop_lim() : 0; }

    inline uint8_t proto() const
    { return ip4h ? ip4h->get_proto() : ip6h ? ip6h->get_next() : 0; }

    inline uint16_t len() const
    { return ip4h ? ip4h->get_len() : ip6h ? ip6h->get_len() : 0; }

    inline uint8_t hlen() const
    { return ip4h ? ip4h->get_hlen() : ip6h ? ip6h->get_hlen() : 0; }

    inline uint8_t ver() const
    { return ip4h ? ip4h->get_ver() : ip6h ? ip6h->get_ver() : 0; }

    inline uint32_t get_ip4_src() const
    { return ip4h ? ip4h->get_src() : 0; }

    inline uint32_t get_ip4_dst() const
    { return ip4h ? ip4h->get_dst() : 0; }

    inline const ipv6::snort_in6_addr* get_ip6_src() const
    { return ip6h ? ip6h->get_src() : nullptr; }

    inline const ipv6::snort_in6_addr* get_ip6_dst() const
    { return ip6h ? ip6h->get_dst() : nullptr; }

private:
    sfip_t src;
    sfip_t dst;
    const sfip_t *src_p;
    const sfip_t *dst_p;
    const IPHdr* ip4h;
    const ipv6::IP6RawHdr* ip6h;
};


inline bool operator==(const IpApi& lhs, const IpApi& rhs)
{ return (lhs.ip4h == rhs.ip4h) && (lhs.ip6h == rhs.ip6h); }

inline bool operator!=(const IpApi& lhs, const IpApi& rhs)
{ return !(lhs == rhs); }


} // namespace ip

#endif
