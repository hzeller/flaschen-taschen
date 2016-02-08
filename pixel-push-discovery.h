/*  -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; -*-
 *  Universal Discovery Protocol
 *  A UDP protocol for finding Etherdream/Heroic Robotics lighting devices
 *
 *  (c) 2012 Jas Strong and Jacob Potter
 *  <jasmine@electronpusher.org> <jacobdp@gmail.com>
 */
#ifndef PIXELPUSHER_UNIVERSAL_DISCOVERY_PROTOCOL_H
#define PIXELPUSHER_UNIVERSAL_DISCOVERY_PROTOCOL_H

#include <stdint.h>

#ifdef __GNUC__
# define PACKED __attribute__((__packed__))
#else
# define PACKED
#endif

#define SFLAG_RGBOW          (1<<0)  // High CRI strip
#define SFLAG_WIDEPIXELS     (1<<1)  // 48 Bit/pixel RGBrgb
#define SFLAG_LOGARITHMIC    (1<<2)  // LED has logarithmic response.
#define SFLAG_MOTION         (1<<3)  // A motion controller.
#define SFLAG_NOTIDEMPOTENT  (1<<4)  // motion controller with side-effects.

#define PFLAG_PROTECTED (1<<0) // require qualified registry.getStrips() call.
#define PFLAG_FIXEDSIZE (1<<1) // Requires every datagram same size.

typedef enum DeviceType {
    ETHERDREAM = 0,
    LUMIABRIDGE = 1,
    PIXELPUSHER = 2
} DeviceType;

// This is slightly modified from the original to separate the dynamic from the
// static parts.
// All multi-byte values here are Little-Endian (read: host byte order).
struct PixelPusherBase {
    uint8_t  strips_attached;
    uint8_t  max_strips_per_packet;
    uint16_t pixels_per_strip;  // uint16_t used to make alignment work
    uint32_t update_period; // in microseconds
    uint32_t power_total;   // in PWM units
    uint32_t delta_sequence;  // difference between received and expected sequence numbers
    int32_t controller_ordinal; // ordering number for this controller.
    int32_t group_ordinal;      // group number for this controller.
    uint16_t artnet_universe;   // configured artnet starting point for this controller
    uint16_t artnet_channel;
    uint16_t my_port;
    // The following has a dynamic length: max(8, strips_attached). So this
    // PixelPusherBase can grow beyond its limits. For the dynamic case, we
    // just allocate more and write beyond the 8
    uint8_t strip_flags[8];     // flags for each strip, for up to eight strips
} PACKED;

struct PixelPusherExt {
    uint16_t padding_;          // The following is read by 32+stripFlagSize
    uint32_t pusher_flags;      // flags for the whole pusher
    uint32_t segments;          // number of segments in each strip
    uint32_t power_domain;      // power domain of this pusher
    uint8_t last_driven_ip[4];  // last host to drive this pusher
    uint16_t last_driven_port;  // source port of last update
} PACKED;

struct PixelPusherContainer {
    struct PixelPusherBase *base;  // dynamically sized.
    struct PixelPusherExt  ext;
};
static inline size_t CalcPixelPusherBaseSize(int num_strips) {
    return sizeof(struct PixelPusherBase) + (num_strips < 8 ? 0 : num_strips-8);
}

typedef struct StaticSizePixelPusher {
    struct PixelPusherBase base;   // Good for up to 8 strips.
    struct PixelPusherExt  ext;
} PACKED PixelPusher;

typedef struct LumiaBridge {
    // placekeeper
} LumiaBridge;

typedef struct EtherDream {
    uint16_t buffer_capacity;
    uint32_t max_point_rate;
    uint8_t light_engine_state;
    uint8_t playback_state;
    uint8_t source;     //   0 = network
    uint16_t light_engine_flags;
    uint16_t playback_flags;
    uint16_t source_flags;
    uint16_t buffer_fullness;
    uint32_t point_rate;                // current point playback rate
    uint32_t point_count;           //  # points played
} EtherDream;

typedef union {
    PixelPusher pixelpusher;
    LumiaBridge lumiabridge;
    EtherDream etherdream;
} Particulars;

typedef struct DiscoveryPacketHeader {
    uint8_t mac_address[6];
    uint8_t ip_address[4];  // network byte order
    uint8_t device_type;
    uint8_t protocol_version; // for the device, not the discovery
    uint16_t vendor_id;
    uint16_t product_id;
    uint16_t hw_revision;
    uint16_t sw_revision;
    uint32_t link_speed;    // in bits per second
} PACKED DiscoveryPacketHeader;

typedef struct DiscoveryPacket {
    DiscoveryPacketHeader header;
    Particulars p;
} DiscoveryPacket;

#endif /* UNIVERSAL_DISCOVERY_PROTOCOL_H */
