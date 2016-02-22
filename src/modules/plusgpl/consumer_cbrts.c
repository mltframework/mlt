/*
 * consumer_cbrts.c -- output constant bitrate MPEG-2 transport stream
 *
 * Copyright (C) 2010-2015 Broadcasting Center Europe S.A. http://www.bce.lu
 * an RTL Group Company  http://www.rtlgroup.com
 * Author: Dan Dennedy <dan@dennedy.org>
 * Some ideas and portions come from OpenCaster, Copyright (C) Lorenzo Pallara <l.pallara@avalpa.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <framework/mlt.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif
#include <strings.h>
// includes for socket IO
#if (_POSIX_C_SOURCE >= 1 || _XOPEN_SOURCE || _POSIX_SOURCE) && (_POSIX_TIMERS > 0)
#if !(defined(__FreeBSD_kernel__) && defined(__GLIBC__))
#define CBRTS_BSD_SOCKETS  1
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <net/if.h>
#include <netinet/in.h>
#endif
#endif
#include <sys/time.h>
#include <time.h>

#define TSP_BYTES     (188)
#define MAX_PID       (8192)
#define SCR_HZ        (27000000ULL)
#define NULL_PID      (0x1fff)
#define PAT_PID       (0)
#define SDT_PID       (0x11)
#define PCR_SMOOTHING (12)
#define PCR_PERIOD_MS (20)
#define RTP_BYTES     (12)
#define UDP_MTU       (RTP_BYTES + TSP_BYTES * 7)
#define REMUX_BUFFER_MIN (10)
#define REMUX_BUFFER_MAX (50)
#define UDP_BUFFER_MINIMUM (100)
#define UDP_BUFFER_DEFAULT (1000)
#define RTP_VERSION   (2)
#define RTP_PAYLOAD   (33)
#define RTP_HZ        (90000)

#define PIDOF( packet )  ( ntohs( *( ( uint16_t* )( packet + 1 ) ) ) & 0x1fff )
#define HASPCR( packet ) ( (packet[3] & 0x20) && (packet[4] != 0) && (packet[5] & 0x10) )
#define CCOF( packet ) ( packet[3] & 0x0f )
#define ADAPTOF( packet ) ( ( packet[3] >> 4 ) & 0x03 )

typedef struct consumer_cbrts_s *consumer_cbrts;

struct consumer_cbrts_s
{
	struct mlt_consumer_s parent;
	mlt_consumer avformat;
	pthread_t thread;
	int joined;
	int running;
	mlt_position last_position;
	mlt_event event_registered;
	int fd;
	uint8_t *leftover_data[TSP_BYTES];
	int leftover_size;
	mlt_deque tsp_packets;
	uint64_t previous_pcr;
	uint64_t previous_packet_count;
	uint64_t packet_count;
	int is_stuffing_set;
	int thread_running;
	uint8_t pcr_count;
	uint16_t pmt_pid;
	int is_si_sdt;
	int is_si_pat;
	int is_si_pmt;
	int dropped;
	uint8_t continuity_count[MAX_PID];
	uint64_t output_counter;
#ifdef CBRTS_BSD_SOCKETS
	struct addrinfo *addr;
	struct timespec timer;
	uint32_t nsec_per_packet;
	uint32_t femto_per_packet;
	uint64_t femto_counter;
#endif
	int ( *write_tsp )( consumer_cbrts, const void *buf, size_t count );
	uint8_t udp_packet[UDP_MTU];
	size_t udp_bytes;
	size_t udp_packet_size;
	mlt_deque udp_packets;
	pthread_t output_thread;
	pthread_mutex_t udp_deque_mutex;
	pthread_cond_t udp_deque_cond;
	uint64_t muxrate;
	int udp_buffer_max;
	uint16_t rtp_sequence;
	uint32_t rtp_ssrc;
	uint32_t rtp_counter;
};

typedef struct {
	int size;
	int period;
	int packet_count;
	uint16_t pid;
	uint8_t data[4096];
} ts_section;

static uint8_t null_packet[ TSP_BYTES ];

/** Forward references to static functions.
*/

static int consumer_start( mlt_consumer parent );
static int consumer_stop( mlt_consumer parent );
static int consumer_is_stopped( mlt_consumer parent );
static void consumer_close( mlt_consumer parent );
static void *consumer_thread( void * );
static void on_data_received( mlt_properties properties, mlt_consumer consumer, uint8_t *buf, int size );

mlt_consumer consumer_cbrts_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	consumer_cbrts self = calloc( sizeof( struct consumer_cbrts_s ), 1 );
	if ( self && mlt_consumer_init( &self->parent, self, profile ) == 0 )
	{
		// Get the parent consumer object
		mlt_consumer parent = &self->parent;

		// Get the properties
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( parent );

		// Create child consumers
		self->avformat = mlt_factory_consumer( profile, "avformat", NULL );
		parent->close = consumer_close;
		parent->start = consumer_start;
		parent->stop = consumer_stop;
		parent->is_stopped = consumer_is_stopped;
		self->joined = 1;
		self->tsp_packets = mlt_deque_init();
		self->udp_packets = mlt_deque_init();

		// Create the null packet
		memset( null_packet, 0xFF, TSP_BYTES );
		null_packet[0] = 0x47;
		null_packet[1] = 0x1f;
		null_packet[2] = 0xff;
		null_packet[3] = 0x10;

		// Create the deque mutex and condition
		pthread_mutex_init( &self->udp_deque_mutex, NULL );
		pthread_cond_init( &self->udp_deque_cond, NULL );

		// Set consumer property defaults
		mlt_properties_set_int( properties, "real_time", -1 );

		return parent;
	}
	free( self );
	return NULL;
}

static ts_section* load_section( const char *filename )
{
	ts_section *section = NULL;
	int fd;

	if ( !filename )
		return NULL;
	if ( ( fd = open( filename, O_RDONLY ) ) < 0 )
	{
		mlt_log_error( NULL, "cbrts consumer failed to load section file %s\n", filename );
		return NULL;
	}

	section = malloc( sizeof(*section) );
	memset( section, 0xff, sizeof(*section) );
	section->size = 0;

	if ( read( fd, section->data, 3 ) )
	{
		// get the size
		uint16_t *p = (uint16_t*) &section->data[1];
		section->size = p[0];
		section->size = ntohs( section->size ) & 0x0FFF;

		// read the data
		if ( section->size <= sizeof(section->data) - 3 )
		{
			ssize_t has_read = 0;
			while ( has_read < section->size )
			{
				ssize_t n = read( fd, section->data + 3 + has_read, section->size );
				if ( n > 0 )
					has_read += n;
				else
					break;
			}
			section->size += 3;
		}
		else
		{
			mlt_log_error( NULL, "Section too big - skipped.\n" );
		}
	}
	close( fd );

	return section;
}

static void load_sections( consumer_cbrts self, mlt_properties properties )
{
	int n = mlt_properties_count( properties );

	// Store the sections with automatic cleanup
	// and make it easy to iterate over the data sections.
	mlt_properties si_properties = mlt_properties_get_data( properties, "si.properties", NULL );
	if ( !si_properties )
	{
		si_properties = mlt_properties_new();
		mlt_properties_set_data( properties, "si.properties", si_properties, 0, (mlt_destructor) mlt_properties_close, NULL );
	}

	while ( n-- )
	{
		// Look for si.<name>.file=filename
		const char *name = mlt_properties_get_name( properties, n );
		if ( strncmp( "si.", name, 3 ) == 0
		  && strncmp( ".file", name + strlen( name ) - 5, 5 ) == 0 )
		{
			size_t len = strlen( name );
			char *si_name = strdup( name + 3 );
			char si_pid[len + 1];

			si_name[len - 3 - 5] = 0;
			strcpy( si_pid, "si." );
			strcat( si_pid, si_name );
			strcat( si_pid, ".pid" );

			// Look for si.<name>.pid=<number>
			if ( mlt_properties_get( properties, si_pid ) )
			{
				char *filename = mlt_properties_get_value( properties, n );

				ts_section *section = load_section( filename );
				if ( section )
				{
					// Determine the periodicity of the section, if supplied
					char si_time[len + 1];

					strcpy( si_time, "si." );
					strcat( si_time, si_name );
					strcat( si_time, ".time" );

					int time = mlt_properties_get_int( properties, si_time );
					if ( time == 0 )
						time = 200;

					// Set flags if we are replacing PAT or SDT
					if ( strncasecmp( "pat", si_name, 3) == 0 )
						self->is_si_pat = 1;
					else if ( strncasecmp( "pmt", si_name, 3) == 0 )
						self->is_si_pmt = 1;
					else if ( strncasecmp( "sdt", si_name, 3) == 0 )
						self->is_si_sdt = 1;

					// Calculate the period and get the PID
					section->period = ( self->muxrate * time ) / ( TSP_BYTES * 8 * 1000 );
					// output one immediately
					section->packet_count = section->period - 1;
					mlt_log_verbose( NULL, "SI %s time=%d period=%d file=%s\n", si_name, time, section->period, filename );
					section->pid = mlt_properties_get_int( properties, si_pid );

					mlt_properties_set_data( si_properties, si_name, section, section->size, free, NULL );
				}
			}
			free( si_name );
		}
	}
}

static void write_section( consumer_cbrts self, ts_section *section )
{
	uint8_t *packet;
	const uint8_t *data_ptr = section->data;
	uint8_t *p;
	int size = section->size;
	int first, len;

	while ( size > 0 )
	{
		first = ( section->data == data_ptr );
		p = packet = malloc( TSP_BYTES );
		*p++ = 0x47;
		*p = ( section->pid >> 8 );
		if ( first )
			*p |= 0x40;
		p++;
		*p++ = section->pid;
		*p++ = 0x10; // continuity count will be written later
		if ( first )
			*p++ = 0; /* 0 offset */
		len = TSP_BYTES - ( p - packet );
		if ( len > size )
			len = size;
		memcpy( p, data_ptr, len );
		p += len;
		/* add known padding data */
		len = TSP_BYTES - ( p - packet );
		if ( len > 0 )
			memset( p, 0xff, len );

		mlt_deque_push_back( self->tsp_packets, packet );
		self->packet_count++;

		data_ptr += len;
		size -= len;
	}
}

static void write_sections( consumer_cbrts self )
{
	mlt_properties properties = mlt_properties_get_data( MLT_CONSUMER_PROPERTIES( &self->parent ), "si.properties", NULL );

	if ( properties )
	{
		int n = mlt_properties_count( properties );
		while ( n-- )
		{
			ts_section *section = mlt_properties_get_data_at( properties, n, NULL );
			if ( ++section->packet_count == section->period )
			{
				section->packet_count = 0;
				write_section( self, section );
			}
		}
	}
}

static uint64_t get_pcr( uint8_t *packet )
{
	uint64_t pcr = 0;
	pcr += (uint64_t) packet[6] << 25;
	pcr += packet[7] << 17;
	pcr += packet[8] << 9;
	pcr += packet[9] << 1;
	pcr += packet[10] >> 7;
	pcr *= 300; // convert 90KHz to 27MHz
	pcr += (packet[10] & 0x01) << 8;
	pcr += packet[11];
	return pcr;
}

static void set_pcr( uint8_t *packet, uint64_t pcr )
{
	uint64_t pcr_base = pcr / 300;
	uint64_t pcr_ext = pcr % 300;
	packet[6]  = pcr_base >> 25;
	packet[7]  = pcr_base >> 17;
	packet[8]  = pcr_base >> 9;
	packet[9]  = pcr_base >> 1;
	packet[10] = (( pcr_base & 1 ) << 7) | 0x7E | (( pcr_ext & 0x100 ) >> 8);
	packet[11] = pcr_ext;
}

static uint64_t update_pcr( consumer_cbrts self, uint64_t muxrate, unsigned packets )
{
	return self->previous_pcr + packets * TSP_BYTES * 8 * SCR_HZ / muxrate;
}

static uint32_t get_rtp_timestamp( consumer_cbrts self )
{
	return self->rtp_counter++ * self->udp_packet_size * 8 * RTP_HZ / self->muxrate;
}

static double measure_bitrate( consumer_cbrts self, uint64_t pcr, int drop )
{
	double muxrate = 0;

	if ( self->is_stuffing_set || self->previous_pcr )
	{
		muxrate = ( self->packet_count - self->previous_packet_count - drop ) * TSP_BYTES * 8;
		if ( pcr >= self->previous_pcr )
			muxrate /= (double) ( pcr - self->previous_pcr ) / SCR_HZ;
		else
			muxrate /= ( ( (double) ( 1ULL << 33 ) - 1 ) * 300 - self->previous_pcr + pcr ) / SCR_HZ;
		mlt_log_debug( NULL, "measured TS bitrate %.1f bits/sec PCR %"PRIu64"\n", muxrate, pcr );
	}

	return muxrate;
}

static int writen( consumer_cbrts self, const void *buf, size_t count )
{
	int result = 0;
	int written = 0;
	while ( written < count )
	{
		if ( ( result = write( self->fd, buf + written, count - written ) ) < 0 )
		{
			mlt_log_error( MLT_CONSUMER_SERVICE(&self->parent), "Failed to write: %s\n", strerror( errno ) );
			break;
		}
		written += result;
	}
	return result;
}

static int sendn( consumer_cbrts self, const void *buf, size_t count )
{
	int result = 0;

#ifdef CBRTS_BSD_SOCKETS
	int written = 0;
	while ( written < count )
	{
		result = sendto(self->fd, buf + written, count - written, 0,
			self->addr->ai_addr, self->addr->ai_addrlen);
		if ( result < 0 )
		{
			mlt_log_error( MLT_CONSUMER_SERVICE(&self->parent), "Failed to send: %s\n", strerror( errno ) );
			exit( EXIT_FAILURE );
			break;
		}
		written += result;
	}
#endif

	return result;
}

static int write_udp( consumer_cbrts self, const void *buf, size_t count )
{
	int result = 0;

#ifdef CBRTS_BSD_SOCKETS
	if ( !self->timer.tv_sec )
		clock_gettime( CLOCK_MONOTONIC, &self->timer );
	self->femto_counter += self->femto_per_packet;
	self->timer.tv_nsec += self->femto_counter / 1000000;
	self->femto_counter  = self->femto_counter % 1000000;
	self->timer.tv_nsec += self->nsec_per_packet;
	self->timer.tv_sec  += self->timer.tv_nsec / 1000000000;
	self->timer.tv_nsec  = self->timer.tv_nsec % 1000000000;
	clock_nanosleep( CLOCK_MONOTONIC, TIMER_ABSTIME, &self->timer, NULL );
	result = sendn( self, buf, count );
#endif

	return result;
}

// socket IO code
static int create_socket( consumer_cbrts self )
{
	int result = -1;

#ifdef CBRTS_BSD_SOCKETS
	struct addrinfo hints = {0};
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( &self->parent );
	const char *hostname = mlt_properties_get( properties, "udp.address" );
	const char *port = "1234";

	if ( mlt_properties_get( properties, "udp.port" ) )
		port = mlt_properties_get( properties, "udp.port" );

	// Resolve the address string and port.
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_family = AF_UNSPEC;
	hints.ai_flags = AI_PASSIVE;
	result = getaddrinfo( hostname, port, &hints, &self->addr );
	if ( result < 0 )
	{
		mlt_log_error( MLT_CONSUMER_SERVICE(&self->parent),
			"Error resolving UDP address and port: %s.\n", gai_strerror( result ) );
		return result;
	}

	// Create the socket descriptor.
	struct addrinfo *addr = self->addr;
	for ( ; addr; addr = addr->ai_next ) {
		result = socket( addr->ai_addr->sa_family, SOCK_DGRAM, addr->ai_protocol );
		if ( result != -1 )
		{
			// success
			self->fd = result;
			result = 0;
			break;
		}
	}
	if ( result < 0 )
	{
		mlt_log_error( MLT_CONSUMER_SERVICE(&self->parent),
			"Error creating socket: %s.\n", strerror( errno ) );
		freeaddrinfo( self->addr ); self->addr = NULL;
		return result;
	}

	// Set the reuse address socket option if not disabled (explicitly set 0).
	int reuse = mlt_properties_get_int( properties, "udp.reuse" ) ||
				!mlt_properties_get( properties, "udp.reuse" );
	if ( reuse )
	{
		result = setsockopt( self->fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse) );
		if ( result < 0 )
		{
			mlt_log_error( MLT_CONSUMER_SERVICE(&self->parent),
				"Error setting the reuse address socket option.\n" );
			close( self->fd );
			freeaddrinfo( self->addr ); self->addr = NULL;
			return result;
		}
	}

	// Set the socket buffer size if supplied.
	if ( mlt_properties_get( properties, "udp.sockbufsize" ) )
	{
		int sockbufsize = mlt_properties_get_int( properties, "udp.sockbufsize" );
		result = setsockopt( self->fd, SOL_SOCKET, SO_SNDBUF, &sockbufsize, sizeof(sockbufsize) );
		if ( result < 0 )
		{
			mlt_log_error( MLT_CONSUMER_SERVICE(&self->parent),
				"Error setting the socket buffer size.\n" );
			close( self->fd );
			freeaddrinfo( self->addr ); self->addr = NULL;
			return result;
		}
	}

	// Set the multicast TTL if supplied.
	if ( mlt_properties_get( properties, "udp.ttl" ) )
	{
		int ttl = mlt_properties_get_int( properties, "udp.ttl" );
		if ( addr->ai_addr->sa_family == AF_INET )
		{
			result = setsockopt( self->fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl) );
		}
		else if ( addr->ai_addr->sa_family == AF_INET6 )
		{
			result = setsockopt( self->fd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &ttl, sizeof(ttl) );
		}
		if ( result < 0 )
		{
			mlt_log_error( MLT_CONSUMER_SERVICE(&self->parent),
				"Error setting the multicast TTL.\n" );
			close( self->fd );
			freeaddrinfo( self->addr ); self->addr = NULL;
			return result;
		}
	}

	// Set the multicast interface if supplied.
	if ( mlt_properties_get( properties, "udp.interface" ) )
	{
		const char *interface = mlt_properties_get( properties, "udp.interface" );
		unsigned int iface = if_nametoindex( interface );

		if ( iface )
		{
			if ( addr->ai_addr->sa_family == AF_INET )
			{
				struct ip_mreqn req = {{0}};
				req.imr_ifindex = iface;
				result = setsockopt( self->fd, IPPROTO_IP, IP_MULTICAST_IF, &req, sizeof(req) );
			}
			else if ( addr->ai_addr->sa_family == AF_INET6 )
			{
				result = setsockopt( self->fd, IPPROTO_IPV6, IPV6_MULTICAST_IF, &iface, sizeof(iface) );
			}
			if ( result < 0 )
			{
				mlt_log_error( MLT_CONSUMER_SERVICE(&self->parent),
					"Error setting the multicast interface.\n" );
				close( self->fd );
				freeaddrinfo( self->addr ); self->addr = NULL;
				return result;
			}
		}
		else
		{
			mlt_log_warning( MLT_CONSUMER_SERVICE(&self->parent),
				"The network interface \"%s\" was not found.\n", interface );
		}
	}
#endif
	return result;
}

static void *output_thread( void *arg )
{
	consumer_cbrts self = arg;
	int result = 0;

	while ( self->thread_running )
	{
		pthread_mutex_lock( &self->udp_deque_mutex );
		while ( self->thread_running && mlt_deque_count( self->udp_packets ) < 1 )
			pthread_cond_wait( &self->udp_deque_cond, &self->udp_deque_mutex );
		pthread_mutex_unlock( &self->udp_deque_mutex );

		// Dequeue the UDP packets and write them.
		int i = mlt_deque_count( self->udp_packets );
		mlt_log_debug( MLT_CONSUMER_SERVICE(&self->parent), "%s: count %d\n", __FUNCTION__, i );
		while ( self->thread_running && i-- && result >= 0 )
		{
			pthread_mutex_lock( &self->udp_deque_mutex );
			uint8_t *packet = mlt_deque_pop_front( self->udp_packets );
			pthread_cond_broadcast( &self->udp_deque_cond );
			pthread_mutex_unlock( &self->udp_deque_mutex );

			size_t size = self->rtp_ssrc ? RTP_BYTES + self->udp_packet_size : self->udp_packet_size;
			result = write_udp( self, packet, size );
			free( packet );
		}
	}
	return NULL;
}

static int enqueue_udp( consumer_cbrts self, const void *buf, size_t count )
{
	// Append TSP to the UDP packet.
	memcpy( &self->udp_packet[self->udp_bytes], buf, count );
	self->udp_bytes = ( self->udp_bytes + count ) % self->udp_packet_size;

	// Send the UDP packet.
	if ( !self->udp_bytes )
	{
		size_t offset = self->rtp_ssrc ? RTP_BYTES : 0;

		// Duplicate the packet.
		uint8_t *packet = malloc( self->udp_packet_size + offset );
		memcpy( packet + offset, self->udp_packet, self->udp_packet_size );

		// Add the RTP header.
		if ( self->rtp_ssrc ) {
			// Padding, extension, and CSRC count are all 0.
			packet[0]  = RTP_VERSION << 6;
			// Marker bit is 0.
			packet[1]  = RTP_PAYLOAD & 0x7f;
			packet[2]  = (self->rtp_sequence >> 8) & 0xff;
			packet[3]  = (self->rtp_sequence >> 0) & 0xff;
			// Timestamp in next 4 bytes.
			uint32_t timestamp = get_rtp_timestamp( self );
			packet[4]  = (timestamp >> 24) & 0xff;
			packet[5]  = (timestamp >> 16) & 0xff;
			packet[6]  = (timestamp >> 8)  & 0xff;
			packet[7]  = (timestamp >> 0)  & 0xff;
			// SSRC in next 4 bytes.
			packet[8]  = (self->rtp_ssrc >> 24) & 0xff;
			packet[9]  = (self->rtp_ssrc >> 16) & 0xff;
			packet[10] = (self->rtp_ssrc >> 8)  & 0xff;
			packet[11] = (self->rtp_ssrc >> 0)  & 0xff;
			self->rtp_sequence++;
		}

		// Wait for room in the fifo.
		pthread_mutex_lock( &self->udp_deque_mutex );
		while ( self->thread_running && mlt_deque_count( self->udp_packets ) >= self->udp_buffer_max )
			pthread_cond_wait( &self->udp_deque_cond, &self->udp_deque_mutex );

		// Add the packet to the fifo.
		mlt_deque_push_back( self->udp_packets, packet );
		pthread_cond_broadcast( &self->udp_deque_cond );
		pthread_mutex_unlock( &self->udp_deque_mutex );
	}

	return 0;
}

static int insert_pcr( consumer_cbrts self, uint16_t pid, uint8_t cc, uint64_t pcr )
{
	uint8_t packet[ TSP_BYTES ];
    uint8_t *p = packet;

    *p++ = 0x47;
    *p++ = pid >> 8;
    *p++ = pid;
    *p++ = 0x20 | cc;     // Adaptation only
    // Continuity Count field does not increment (see 13818-1 section 2.4.3.3)
    *p++ = TSP_BYTES - 5; // Adaptation Field Length
    *p++ = 0x10;          // Adaptation flags: PCR present
	set_pcr( packet, pcr );
	p += 6; // 6 pcr bytes
    memset( p, 0xff, TSP_BYTES - ( p - packet ) ); // stuffing

	return self->write_tsp( self, packet, TSP_BYTES );
}

static int output_cbr( consumer_cbrts self, uint64_t input_rate, uint64_t output_rate, uint64_t *pcr )
{
	int n = mlt_deque_count( self->tsp_packets );
	unsigned output_packets = 0;
	unsigned packets_since_pcr = 0;
	int result = 0;
	int dropped = 0;
	int warned = 0;
	uint16_t pcr_pid = 0;
	uint8_t cc = 0xff;
	float ms_since_pcr;
	float ms_to_end;
	uint64_t input_counter = 0;
	uint64_t last_input_counter;

	mlt_log_debug( NULL, "%s: n %i output_counter %"PRIu64" input_rate %"PRIu64"\n", __FUNCTION__, n, self->output_counter, input_rate );
	while ( self->thread_running && n-- && result >= 0 )
	{
		uint8_t *packet = mlt_deque_pop_front( self->tsp_packets );
		uint16_t pid = PIDOF( packet );

		// Check for overflow
		if ( input_rate > output_rate && !HASPCR( packet ) &&
		     pid != SDT_PID && pid != PAT_PID && pid != self->pmt_pid )
		{
			if ( !warned )
			{
				mlt_log_warning( MLT_CONSUMER_SERVICE( &self->parent ), "muxrate too low %"PRIu64" > %"PRIu64"\n", input_rate, output_rate );
				warned = 1;
			}

			// Skip this packet
			free( packet );

			// Compute new input_rate based on dropped count
			input_rate = measure_bitrate( self, *pcr, ++dropped );

			continue;
		}

		if ( HASPCR( packet ) )
		{
			pcr_pid = pid;
			set_pcr( packet, update_pcr( self, output_rate, output_packets ) );
			packets_since_pcr = 0;
		}

		// Rewrite the continuity counter if not only adaptation field
		if ( ADAPTOF( packet ) != 2 )
		{
			packet[3] = ( packet[3] & 0xf0 ) | self->continuity_count[ pid ];
			self->continuity_count[ pid ] = ( self->continuity_count[ pid ] + 1 ) & 0xf;
		}
		if ( pcr_pid && pid == pcr_pid )
			cc = CCOF( packet );

		result = self->write_tsp( self, packet, TSP_BYTES );
		free( packet );
		if ( result < 0 )
			break;
		output_packets++;
		packets_since_pcr++;
		self->output_counter += TSP_BYTES * 8 * output_rate;
		input_counter += TSP_BYTES * 8 * input_rate;

		// See if we need to output a dummy packet with PCR
		ms_since_pcr = (float) ( packets_since_pcr + 1 ) * 8 * TSP_BYTES * 1000 / output_rate;
		ms_to_end = (float) n * 8 * TSP_BYTES * 1000 / input_rate;
		if ( pcr_pid && ms_since_pcr >= PCR_PERIOD_MS && ms_to_end > PCR_PERIOD_MS / 2.0 )
		{
			uint64_t new_pcr = update_pcr( self, output_rate, output_packets );
			if ( ms_since_pcr > 40 )
				mlt_log_warning( NULL, "exceeded PCR interval %.2f ms queued %.2f ms\n", ms_since_pcr, ms_to_end );
			if ( ( result = insert_pcr( self, pcr_pid, cc, new_pcr ) ) < 0 )
				break;
			packets_since_pcr = 0;
			output_packets++;
			input_counter += TSP_BYTES * 8 * input_rate;
		}

		// Output null packets as needed
		while ( self->thread_running && input_counter + ( TSP_BYTES * 8 * input_rate ) <= self->output_counter )
		{
			// See if we need to output a dummy packet with PCR
			ms_since_pcr = (float) ( packets_since_pcr + 1 ) * 8 * TSP_BYTES * 1000 / output_rate;
			ms_to_end = (float) n * 8 * TSP_BYTES * 1000 / input_rate;

			if ( pcr_pid && ms_since_pcr >= PCR_PERIOD_MS && ms_to_end > PCR_PERIOD_MS / 2.0 )
			{
				uint64_t new_pcr = update_pcr( self, output_rate, output_packets );
				if ( ms_since_pcr > 40 )
					mlt_log_warning( NULL, "exceeded PCR interval %.2f ms queued %.2f ms\n", ms_since_pcr, ms_to_end );
				if ( ( result = insert_pcr( self, pcr_pid, cc, new_pcr ) ) < 0 )
					break;
				packets_since_pcr = 0;
			}
			else
			{
				// Otherwise output a null packet
				if ( ( result = self->write_tsp( self, null_packet, TSP_BYTES ) ) < 0 )
					break;
				packets_since_pcr++;
			}
			output_packets++;

			// Increment input
			last_input_counter = input_counter;
			input_counter += TSP_BYTES * 8 * input_rate;

			// Handle wrapping
			if ( last_input_counter > input_counter )
			{
				last_input_counter -= self->output_counter;
				self->output_counter = 0;
				input_counter = last_input_counter + TSP_BYTES * 8 * input_rate;
			}
		}
	}

	// Reset counters leaving a residual output count
	if ( input_counter < self->output_counter )
		self->output_counter -= input_counter;
	else
		self->output_counter = 0;

	// Warn if the PCR interval is too large or small
	ms_since_pcr = (float) packets_since_pcr * 8 * TSP_BYTES * 1000 / output_rate;
	if ( ms_since_pcr > 40 )
		mlt_log_warning( NULL, "exceeded PCR interval %.2f ms\n", ms_since_pcr );
	else if ( ms_since_pcr < PCR_PERIOD_MS / 2.0 )
		mlt_log_debug( NULL, "PCR interval too short %.2f ms\n", ms_since_pcr );

	// Update the current PCR based on number of packets output
	*pcr = update_pcr( self, output_rate, output_packets );

	return result;
}

static void get_pmt_pid( consumer_cbrts self, uint8_t *packet )
{
	// Skip 5 bytes of TSP header + 8 bytes of section header + 2 bytes of service ID
	uint16_t *p = ( uint16_t* )( packet + 5 + 8 + 2 );
	self->pmt_pid = ntohs( p[0] ) & 0x1fff;
	mlt_log_debug(NULL, "PMT PID 0x%04x\n", self->pmt_pid );
	return;
}

static int remux_packet( consumer_cbrts self, uint8_t *packet )
{
	mlt_service service = MLT_CONSUMER_SERVICE( &self->parent );
	uint16_t pid = PIDOF( packet );
	int result = 0;

	write_sections( self );

	// Sanity checks
	if ( packet[0] != 0x47 )
	{
		mlt_log_panic( service, "NOT SYNC BYTE 0x%02x\n", packet[0] );
		exit(1);
	}
	if ( pid == NULL_PID )
	{
		mlt_log_panic( service, "NULL PACKET\n" );
		exit(1);
	}

	// Measure the bitrate between consecutive PCRs
	if ( HASPCR( packet ) )
	{
		if ( self->pcr_count++ % PCR_SMOOTHING == 0 )
		{
			uint64_t pcr = get_pcr( packet );
			double input_rate = measure_bitrate( self, pcr, 0 );
			if ( input_rate > 0 )
			{
				self->is_stuffing_set = 1;
				if ( input_rate > 1.0 )
				{
					result = output_cbr( self, input_rate, self->muxrate, &pcr );
					set_pcr( packet, pcr );
				}
			}
			self->previous_pcr = pcr;
			self->previous_packet_count = self->packet_count;
		}
	}
	mlt_deque_push_back( self->tsp_packets, packet );
	self->packet_count++;
	return result;
}

static void start_output_thread( consumer_cbrts self )
{
	int rtprio = mlt_properties_get_int( MLT_CONSUMER_PROPERTIES( &self->parent ), "udp.rtprio" );
	self->thread_running = 1;
	if ( rtprio > 0 )
	{
		// Use realtime priority class
		struct sched_param priority;
		pthread_attr_t thread_attributes;
		pthread_attr_init( &thread_attributes );
		priority.sched_priority = rtprio;
		pthread_attr_setschedpolicy( &thread_attributes, SCHED_FIFO );
		pthread_attr_setschedparam( &thread_attributes, &priority );
		pthread_attr_setinheritsched( &thread_attributes, PTHREAD_EXPLICIT_SCHED );
		pthread_attr_setscope( &thread_attributes, PTHREAD_SCOPE_SYSTEM );
		if ( pthread_create( &self->output_thread, &thread_attributes, output_thread, self ) < 0 )
		{
			mlt_log_info( MLT_CONSUMER_SERVICE( &self->parent ),
				"failed to initialize output thread with realtime priority\n" );
			pthread_create( &self->output_thread, &thread_attributes, output_thread, self );
		}
		pthread_attr_destroy( &thread_attributes );
	}
	else
	{
		// Use normal priority class
		pthread_create( &self->output_thread, NULL, output_thread, self );
	}
}

static void stop_output_thread( consumer_cbrts self )
{
	self->thread_running = 0;

	// Broadcast to the condition in case it's waiting.
	pthread_mutex_lock( &self->udp_deque_mutex );
	pthread_cond_broadcast( &self->udp_deque_cond );
	pthread_mutex_unlock( &self->udp_deque_mutex );

	// Join the thread.
	pthread_join( self->output_thread, NULL );

	// Release the buffered packets.
	pthread_mutex_lock( &self->udp_deque_mutex );
	int n = mlt_deque_count( self->udp_packets );
	while ( n-- )
		free( mlt_deque_pop_back( self->udp_packets ) );
	pthread_mutex_unlock( &self->udp_deque_mutex );
}

static inline int filter_packet( consumer_cbrts self, uint8_t *packet )
{
	uint16_t pid = PIDOF( packet );

	// We are going to keep the existing PMT; replace all other signaling sections.
	int result = ( pid == NULL_PID )
	          || ( pid == PAT_PID && self->is_si_pat )
	          || ( pid == self->pmt_pid && self->is_si_pmt )
	          || ( pid == SDT_PID && self->is_si_sdt );

	// Get the PMT PID from the PAT
	if ( pid == PAT_PID && !self->pmt_pid )
	{
		get_pmt_pid( self, packet );
		if ( self->is_si_pmt )
			result = 1;
	}

	return result;
}

static void filter_remux_or_write_packet( consumer_cbrts self, uint8_t *packet )
{
	int remux = !mlt_properties_get_int( MLT_CONSUMER_PROPERTIES( &self->parent ), "noremux" );

	// Filter out packets
	if ( remux ) {
		if ( !filter_packet( self, packet ) )
			remux_packet( self, packet );
		else
			free( packet );
	} else {
		self->write_tsp( self, packet, TSP_BYTES );
		free( packet );
	}
}

static void on_data_received( mlt_properties properties, mlt_consumer consumer, uint8_t *buf, int size )
{
	if ( size > 0 )
	{
		consumer_cbrts self = (consumer_cbrts) consumer->child;

		// Sanity check
		if ( self->leftover_size == 0 && buf[0] != 0x47 )
		{
			mlt_log_verbose(MLT_CONSUMER_SERVICE( consumer ), "NOT SYNC BYTE 0x%02x\n", buf[0] );
			while ( size && buf[0] != 0x47 )
			{
				buf++;
				size--;
			}
			if ( size <= 0 )
				exit(1);
		}

		// Enqueue the packets
		int num_packets = ( self->leftover_size + size ) / TSP_BYTES;
		int remaining = ( self->leftover_size + size ) % TSP_BYTES;
		uint8_t *packet = NULL;
		int i;

//			mlt_log_verbose( MLT_CONSUMER_SERVICE(consumer), "%s: packets %d remaining %i\n", __FUNCTION__, num_packets, self->leftover_size );

		if ( self->leftover_size )
		{
			packet = malloc( TSP_BYTES );
			memcpy( packet, self->leftover_data, self->leftover_size );
			memcpy( packet + self->leftover_size, buf, TSP_BYTES - self->leftover_size );
			buf += TSP_BYTES - self->leftover_size;
			--num_packets;
			filter_remux_or_write_packet( self, packet );
		}
		for ( i = 0; i < num_packets; i++, buf += TSP_BYTES )
		{
			packet = malloc( TSP_BYTES );
			memcpy( packet, buf, TSP_BYTES );
			filter_remux_or_write_packet( self, packet );
		}

		self->leftover_size = remaining;
		memcpy( self->leftover_data, buf, self->leftover_size );

		if ( !self->thread_running )
			start_output_thread( self );
		mlt_log_debug( MLT_CONSUMER_SERVICE(consumer), "%s: %p 0x%x (%d)\n", __FUNCTION__, buf, *buf, size % TSP_BYTES );

		// Do direct output
//		result = self->write_tsp( self, buf, size );
	}
}

static int consumer_start( mlt_consumer parent )
{
	consumer_cbrts self = parent->child;

	if ( !self->running )
	{
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( parent );
		mlt_properties avformat = MLT_CONSUMER_PROPERTIES(self->avformat);

		// Cleanup after a possible abort
		consumer_stop( parent );

		// Pass properties down
		mlt_properties_pass( avformat, properties, "" );
		mlt_properties_set_data( avformat, "app_lock", mlt_properties_get_data( properties, "app_lock", NULL ), 0, NULL, NULL );
		mlt_properties_set_data( avformat, "app_unlock", mlt_properties_get_data( properties, "app_unlock", NULL ), 0, NULL, NULL );
		mlt_properties_set_int( avformat, "put_mode", 1 );
		mlt_properties_set_int( avformat, "real_time", -1 );
		mlt_properties_set_int( avformat, "buffer", 2 );
		mlt_properties_set_int( avformat, "terminate_on_pause", 0 );
		mlt_properties_set_int( avformat, "muxrate", 1 );
		mlt_properties_set_int( avformat, "redirect", 1 );
		mlt_properties_set( avformat, "f", "mpegts" );
		self->dropped = 0;
		self->fd = STDOUT_FILENO;
		self->write_tsp = writen;
		self->muxrate = mlt_properties_get_int64( MLT_CONSUMER_PROPERTIES(&self->parent), "muxrate" );

		if ( mlt_properties_get( properties, "udp.address" ) )
		{
			if ( create_socket( self ) >= 0 )
			{
				int is_rtp = 1;
				if ( mlt_properties_get( properties, "udp.rtp" ) )
					is_rtp = !!mlt_properties_get_int( properties, "udp.rtp" );
				if ( is_rtp ) {
					self->rtp_ssrc = mlt_properties_get_int( properties, "udp.rtp_ssrc" );
					while ( !self->rtp_ssrc )
						self->rtp_ssrc = (uint32_t) rand();
					self->rtp_counter = (uint32_t) rand();
				}

				self->udp_packet_size = mlt_properties_get_int( properties, "udp.nb_tsp" ) * TSP_BYTES;
				if ( self->udp_packet_size <= 0 || self->udp_packet_size > UDP_MTU )
					self->udp_packet_size = 7 * TSP_BYTES;
#ifdef CBRTS_BSD_SOCKETS
				self->nsec_per_packet  = 1000000000UL * self->udp_packet_size * 8 / self->muxrate;
				self->femto_per_packet = 1000000000000000ULL * self->udp_packet_size * 8 / self->muxrate - self->nsec_per_packet * 1000000;
#endif
				self->udp_buffer_max = mlt_properties_get_int( properties, "udp.buffer" );
				if ( self->udp_buffer_max < UDP_BUFFER_MINIMUM )
					self->udp_buffer_max = UDP_BUFFER_DEFAULT;

				self->write_tsp = enqueue_udp;
			}
		}


		// Load the DVB PSI/SI sections
		load_sections( self, properties );

		// Start the FFmpeg consumer and our thread
		mlt_consumer_start( self->avformat );
		pthread_create( &self->thread, NULL, consumer_thread, self );
		self->running = 1;
		self->joined = 0;
	}

	return 0;
}

static int consumer_stop( mlt_consumer parent )
{
	// Get the actual object
	consumer_cbrts self = parent->child;

	if ( !self->joined )
	{
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( parent );
		int app_locked = mlt_properties_get_int( properties, "app_locked" );
		void ( *lock )( void ) = mlt_properties_get_data( properties, "app_lock", NULL );
		void ( *unlock )( void ) = mlt_properties_get_data( properties, "app_unlock", NULL );

		if ( app_locked && unlock ) unlock( );

		// Kill the threads and clean up
		self->running = 0;
#ifndef _WIN32
		if ( self->thread )
#endif
			pthread_join( self->thread, NULL );
		self->joined = 1;

		if ( self->avformat )
			mlt_consumer_stop( self->avformat );
		stop_output_thread( self );
		if ( self->fd > 1 )
			close( self->fd );

		if ( app_locked && lock ) lock( );
	}
	return 0;
}

static int consumer_is_stopped( mlt_consumer parent )
{
	consumer_cbrts self = parent->child;
	return !self->running;
}

static void *consumer_thread( void *arg )
{
	// Identify the arg
	consumer_cbrts self = arg;

	// Get the consumer and producer
	mlt_consumer consumer = &self->parent;

	// internal intialization
	mlt_frame frame = NULL;
	int last_position = -1;

	// Loop until told not to
	while( self->running )
	{
		// Get a frame from the attached producer
		frame = mlt_consumer_rt_frame( consumer );

		// Ensure that we have a frame
		if ( self->running && frame )
		{
			if ( mlt_properties_get_int( MLT_FRAME_PROPERTIES( frame ), "rendered" ) != 1 )
			{
				mlt_frame_close( frame );
				mlt_log_warning( MLT_CONSUMER_SERVICE(consumer), "dropped frame %d\n", ++self->dropped );
				continue;
			}

			// Get the speed of the frame
			double speed = mlt_properties_get_double( MLT_FRAME_PROPERTIES( frame ), "_speed" );

			// Optimisation to reduce latency
			if ( speed == 1.0 )
			{
				if ( last_position != -1 && last_position + 1 != mlt_frame_get_position( frame ) )
					mlt_consumer_purge( self->avformat );
				last_position = mlt_frame_get_position( frame );
			}
			else
			{
				//mlt_consumer_purge( this->play );
				last_position = -1;
			}

			mlt_consumer_put_frame( self->avformat, frame );

			// Setup event listener as a callback from consumer avformat
			if ( !self->event_registered )
				self->event_registered = mlt_events_listen( MLT_CONSUMER_PROPERTIES( self->avformat ),
					consumer, "avformat-write", (mlt_listener) on_data_received );
		}
		else
		{
			if ( frame ) mlt_frame_close( frame );
			mlt_consumer_put_frame( self->avformat, NULL );
			self->running = 0;
		}
	}
	return NULL;
}

/** Callback to allow override of the close method.
*/

static void consumer_close( mlt_consumer parent )
{
	// Get the actual object
	consumer_cbrts self = parent->child;

	// Stop the consumer
	mlt_consumer_stop( parent );

	// Close the child consumers
	mlt_consumer_close( self->avformat );

	// Now clean up the rest
	mlt_deque_close( self->tsp_packets );
	mlt_deque_close( self->udp_packets );
	mlt_consumer_close( parent );

	// Finally clean up this
	free( self );
}
