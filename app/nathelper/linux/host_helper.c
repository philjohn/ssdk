/*
 * Copyright (c) 2012 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */


#ifdef KVER32
#include <linux/kconfig.h> 
#include <generated/autoconf.h>
#else
#include <linux/autoconf.h>
#endif
#include <linux/if_arp.h>
#include <linux/netdevice.h>
#include <linux/netfilter_arp.h>
#include <linux/inetdevice.h>
#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/tcp.h>
#include <linux/icmp.h>
#ifdef KVER32
#include <linux/export.h>
#endif
#include <net/netfilter/nf_conntrack.h>
#if defined(CONFIG_VLAN_8021Q) || defined(CONFIG_VLAN_8021Q_MODULE)
#include <linux/if_vlan.h>
#endif
#if defined (CONFIG_BRIDGE)
#include <../net/bridge/br_private.h>
#endif
#include <linux/ppp_defs.h>
#include <linux/filter.h>
#include <linux/if_pppox.h>
#include <linux/if_ppp.h>
#include <linux/ppp_channel.h>
#include <linux/ppp-comp.h>
#include <net/sock.h>
#include <net/route.h>
#include <net/netevent.h>
#include <net/ipv6.h>
#include <net/ip_fib.h>
#include "hsl_api.h"
#include "fal_nat.h"
#include "fal_ip.h"
#include "fal_fdb.h"
#include "hsl.h"
#include "nat_helper.h"
#include "napt_acl.h"
#include "lib/nat_helper_hsl.h"
#include "lib/nat_helper_dt.h"
#include "hsl_shared_api.h"
#include <net/addrconf.h>

#ifdef ISISC
#define CONFIG_IPV6_HWACCEL 1
#else
#undef CONFIG_IPV6_HWACCEL
#endif

#ifdef CONFIG_IPV6_HWACCEL
#include <net/ndisc.h>
#include <net/neighbour.h>
#include <net/netevent.h>
#include <net/ipv6.h>
#include <net/ip6_route.h>
#include <linux/ipv6.h>
#include <linux/netfilter_ipv6.h>
#endif

//#define AP136_QCA_HEADER_EN 1
#define MAC_LEN 6
#define IP_LEN 4
#define ARP_HEADER_LEN 8

#define ARP_ENTRY_MAX 128

/* P6 is used by loop dev. */
#define S17_P6PAD_MODE_REG_VALUE 0x01000000

#define MULTIROUTE_WR

extern struct net init_net;

//char *nat_lan_dev_list = "eth0.1";
char *nat_lan_dev_list = "br-lan";
char *nat_wan_dev_list = "eth0.2";
#define NAT_LAN_DEV_VID 1
#define NAT_WAN_DEV_VID 2

static int wan_fid = 0xffff;
static fal_pppoe_session_t pppoetbl = {0};
static uint32_t pppoe_gwid = 0;
static char nat_bridge_dev[IFNAMSIZ*4];
static uint8_t lanip[4] = {0}, wanip[4] = {0};
static struct in6_addr wan6ip = IN6ADDR_ANY_INIT;
static struct in6_addr lan6ip = IN6ADDR_ANY_INIT;

#ifdef ISISC
struct ipv6_default_route_binding
{
    struct in6_addr next_hop;
    uint32_t nh_entry_id;
};
#endif

#ifdef MULTIROUTE_WR
#define MAX_HOST 8
struct wan_next_hop
{
    u_int32_t host_ip;
    u_int32_t entry_id;
    u_int32_t in_acl;
    u_int32_t in_use;
    u_int8_t  host_mac[6];
};
static struct net_device *multi_route_indev = NULL;
static struct wan_next_hop wan_nh_ent[MAX_HOST] = {{0}};

static int wan_nh_get(u_int32_t host_ip)
{
    int i;

    host_ip = htonl(host_ip);

    for (i=0; i<MAX_HOST; i++)
    {
        if ((wan_nh_ent[i].host_ip != 0) && !memcmp(&wan_nh_ent[i].host_ip, &host_ip, 4))
        {
            // printk("%s %d\n", __FUNCTION__, __LINE__);
            // if ((wan_nh_ent[i].entry_id != 0) && (wan_nh_ent[i].in_acl != 1))
            if (wan_nh_ent[i].in_acl != 1)
            {
                wan_nh_ent[i].in_acl = 1;

                return i;
            }
            // printk("%s %d\n", __FUNCTION__, __LINE__);
        }
        printk("%s %d wan_nh_ent 0x%08x host_ip 0x%08x\n", __FUNCTION__, __LINE__, wan_nh_ent[i].host_ip, host_ip);
    }

    return -1;
}

static void wan_nh_add(u_int8_t *host_ip , u_int8_t *host_mac, u_int32_t id)
{
    int i;

    for( i = 0 ; i < MAX_HOST ; i++ )
    {
        if((wan_nh_ent[i].host_ip != 0) && !memcmp(&wan_nh_ent[i].host_ip, host_ip, 4))
        {
            if (host_mac == NULL) break;

            if(!memcmp(&wan_nh_ent[i].host_mac, host_mac,6))
                return;
            else
                break ;
        }

        if(wan_nh_ent[i].host_ip == 0)
            break;
    }

    if (i < MAX_HOST)
    {
        if ((wan_nh_ent[i].in_use) && (wan_nh_ent[i].in_acl)) return;

        memcpy(&wan_nh_ent[i].host_ip, host_ip, 4);
        if (host_mac != NULL)
        {
            memcpy(wan_nh_ent[i].host_mac, host_mac, 6);
            wan_nh_ent[i].entry_id = id;
            if ((wan_nh_ent[i].in_use) && !(wan_nh_ent[i].in_acl))
            {
                droute_add_acl_rules(*(uint32_t *)&lanip, id);
                /* set the in_acl flag */
                wan_nh_ent[i].in_acl = 1;
            }
        }
        else
        {
            /* set the in_use flag */
            wan_nh_ent[i].in_use = 1;
        }
        aos_printk("%s: ip %08x (%d)\n" ,__func__, wan_nh_ent[i].host_ip, i);
    }
}

static uint32_t get_next_hop( uint32_t daddr , uint32_t saddr )
{
    struct fib_result res;
#ifdef KVER32
    struct flowi4 fl =
    {
        .flowi4_iif =  multi_route_indev->ifindex,
        .flowi4_mark = 0,
        .flowi4_tos = 0,
        .flowi4_scope = RT_SCOPE_UNIVERSE,
        .daddr = htonl(daddr),
        .saddr = htonl(saddr),
    };
#else
    struct flowi fl = { .nl_u = { .ip4_u =
            {
                .daddr = daddr,
                .saddr = saddr,
                .tos = 0,
                .scope = RT_SCOPE_UNIVERSE,
            }
        },
        .mark = 0,
        .iif = multi_route_indev->ifindex
    };
#endif
    struct net    * net = dev_net(multi_route_indev);
    struct fib_nh *mrnh = NULL;

    if (fib_lookup(net, &fl, &res) != 0)
    {
        return 0;
    }
    else
    {
        mrnh = res.fi->fib_nh;
        if (NULL == mrnh)
        {
            return 0;
        }
    }

    return ntohl(mrnh->nh_gw);
}

uint32_t napt_set_default_route(fal_ip4_addr_t dst_addr, fal_ip4_addr_t src_addr)
{
    sw_error_t rv;

    /* search for the next hop (s) */
    if (!(get_aclrulemask() & (1 << S17_ACL_LIST_DROUTE)))
    {
        if (multi_route_indev && \
                (nf_athrs17_hnat_wan_type != NF_S17_WAN_TYPE_PPPOE) &&
                (nf_athrs17_hnat_wan_type != NF_S17_WAN_TYPE_PPPOES0))
        {
            uint32_t next_hop = get_next_hop(dst_addr, src_addr);

            HNAT_PRINTK("Next hop: %08x\n", next_hop);
            if (next_hop != 0)
            {
                fal_host_entry_t arp_entry;

                memset(&arp_entry, 0, sizeof(arp_entry));
                arp_entry.ip4_addr = next_hop;
                arp_entry.flags = FAL_IP_IP4_ADDR;
                rv = IP_HOST_GET(0, FAL_IP_ENTRY_IPADDR_EN, &arp_entry);
                if (rv != SW_OK)
                {
                    printk("%s: IP_HOST_GET error... (non-existed host: %08x?) \n", __func__, next_hop);
                    /* add into the nh_ent */
                    wan_nh_add((u_int8_t *)&next_hop, (u_int8_t *)NULL, 0);
                }
                else
                {
                    if (wan_nh_get(next_hop) != -1)
                        droute_add_acl_rules(*(uint32_t *)&lanip, arp_entry.entry_id);
                    else
                        printk("%s %d\n", __FUNCTION__, __LINE__);
                }
            }
            else
            {
                HNAT_PRINTK("no need to set the default route... \n");
                // set_aclrulemask (S17_ACL_LIST_DROUTE);
            }
        }
        else
        {
            printk("multi_route_indev %p nf_athrs17_hnat_wan_type %d\n", multi_route_indev, nf_athrs17_hnat_wan_type);
        }
    }
    /* end next hop (s) */

    return SW_OK;
}
#endif /* MULTIROUTE_WR */

static void qcaswitch_hostentry_flush(void)
{
    fal_host_entry_t hostentry;
    sw_error_t ret;

    do
    {
        memset(&hostentry, 0, sizeof(fal_host_entry_t));
        hostentry.entry_id = FAL_NEXT_ENTRY_FIRST_ID;
        ret = IP_HOST_NEXT (0, FAL_IP_ENTRY_ID_EN, &hostentry);
        if (SW_OK == ret)
        {
            IP_HOST_DEL(0, FAL_IP_ENTRY_IPADDR_EN, &hostentry);
        }
    }while (SW_OK == ret);
    
    return;
}

#ifdef CONFIG_IPV6_HWACCEL /* only for S17c */
static struct in6_addr* get_ipv6_default_gateway(void)
{
    /* ip_route_output_key can't return correct default nexhop
     * routes are less than 4 and it only searches in route
     * hash, not in fib, so use fib_lookup.
     */
    struct in6_addr *ip6addr = NULL;
    struct in6_addr des_addr = IN6ADDR_ANY_INIT;
    struct rt6_info *rt = rt6_lookup(&init_net, &des_addr, NULL, 0, 0);

    if (rt)
    {
        ip6addr = &rt->rt6i_gateway;
    }

    return ip6addr;
}

static int add_pppoev6_host_entry(void)
{
    struct in6_addr local_lan6ip = IN6ADDR_ANY_INIT;
    unsigned long  flags;
    int ppp_sid, ppp_sid2;
    unsigned char ppp_peer_mac[ETH_ALEN];
    unsigned char ppp_peer_mac2[ETH_ALEN];
    a_uint32_t ppp_peer_ip = 0;
    int wvid;
    fal_host_entry_t nh_arp_entry;
    sw_error_t rv;
    a_uint32_t droute_entry_id = 0;
    a_bool_t ena;
    static fal_pppoe_session_t pppoev6_sid_table = {0};
    struct  in6_addr *next_hop;

    local_irq_save(flags);
    memcpy(&local_lan6ip, &lan6ip, sizeof(struct in6_addr));
    ppp_sid2 = nf_athrs17_hnat_ppp_id2;
    ppp_sid = nf_athrs17_hnat_ppp_id;
    ppp_peer_ip = nf_athrs17_hnat_ppp_peer_ip;
    memcpy(ppp_peer_mac, nf_athrs17_hnat_ppp_peer_mac, ETH_ALEN);
    memcpy(ppp_peer_mac2, nf_athrs17_hnat_ppp_peer_mac2, ETH_ALEN);
    wvid = wan_fid;
    local_irq_restore(flags);

    if (NF_S17_WAN_TYPE_PPPOE != nf_athrs17_hnat_wan_type)
    {
        return SW_BAD_STATE;
    }

    if (__ipv6_addr_type(&local_lan6ip) == IPV6_ADDR_ANY)
    {
        /* Cannot get lanip6 successfully. */
        return SW_BAD_STATE;
    }
    if (0xffff == wvid)
    {
        printk("%s: Cannot get WAN vid!\n", __FUNCTION__);
        return SW_FAIL;
    }

    if (0 == nf_athrs17_hnat_ppp_peer_ip)
    {
        return SW_FAIL;
    }

    next_hop = get_ipv6_default_gateway();
    if (NULL == next_hop)
    {
        printk("No IPv6 Gateway!\n");
        return SW_BAD_STATE;
    }

    if (0 != ppp_sid)
    {
        if ((ppp_sid == ppp_sid2)||(0 == ppp_sid2)) /* v4 and v6 have the same session id */
        {
            memset(&nh_arp_entry, 0, sizeof(nh_arp_entry));
            nh_arp_entry.ip4_addr = ppp_peer_ip;
            nh_arp_entry.flags = FAL_IP_IP4_ADDR;
            rv = IP_HOST_GET(0, FAL_IP_ENTRY_IPADDR_EN, &nh_arp_entry);
            if (rv != SW_OK)
            {
                printk("%s: IP_HOST_GET error (0x%08x)\n", __func__, ppp_peer_ip);
                if (PPPOE_STATUS_GET(0, &ena) != SW_OK)
                {
                    if (!ena)
                    {
                        if (PPPOE_STATUS_SET(0, A_TRUE) != SW_OK)
                        {
                            aos_printk("Cannot enable the PPPoE mode\n");
                            return SW_FAIL;
                        }
                    }
                }
                pppoev6_sid_table.session_id = ppp_sid;
                pppoev6_sid_table.multi_session = 1;
                pppoev6_sid_table.uni_session = 1;
                pppoev6_sid_table.entry_id = 0;
                /* set the PPPoE edit reg (0x2200), and PPPoE session reg (0x5f000) */
                rv = PPPOE_SESSION_TABLE_ADD(0, &pppoev6_sid_table);
                if (rv == SW_OK)
                {
                    a_int32_t a_entry_id = -1;

                    PPPOE_SESSION_ID_SET(0, pppoev6_sid_table.entry_id, pppoev6_sid_table.session_id);
                    aos_printk("pppoe session: %d, entry_id: %d\n",
                               pppoev6_sid_table.session_id, pppoev6_sid_table.entry_id);
                    /* create the peer host ARP entry */
                    a_entry_id = arp_hw_add(S17_WAN_PORT, wan_fid, (void *)&ppp_peer_ip, (void *)ppp_peer_mac, 0);
                    if (a_entry_id >= 0) /* hostentry creation okay */
                    {
                        rv = IP_HOST_PPPOE_BIND(0, a_entry_id, pppoev6_sid_table.entry_id, A_TRUE);
                        if ( rv != SW_OK)
                        {
                            aos_printk("IP_HOST_PPPOE_BIND failed (entry: %d, rv: %d)... \n",
                                       a_entry_id, rv);
                            PPPOE_SESSION_TABLE_DEL(0, &pppoev6_sid_table);
                            return SW_FAIL;
                        }
                        droute_entry_id = a_entry_id;
                    }
                    else
                    {
                        PPPOE_SESSION_TABLE_DEL(0, &pppoev6_sid_table);
                        return SW_FAIL;
                    }
                }
                else
                {
                    aos_printk("PPPoE session add failed.. (id: %d)\n",
                               pppoev6_sid_table.session_id);
                    aos_printk("rv: %d\n", rv);
                    return SW_FAIL;
                }
            }
            else
            {
                droute_entry_id = nh_arp_entry.entry_id;
            }
            ipv6_droute_add_acl_rules(&local_lan6ip, droute_entry_id);
        }
        else /* Not the same session id */
        {
            if (PPPOE_STATUS_GET(0, &ena) != SW_OK)
            {
                if (!ena)
                {
                    if (PPPOE_STATUS_SET(0, A_TRUE) != SW_OK)
                    {
                        aos_printk("Cannot enable the PPPoE mode\n");
                        return SW_FAIL;
                    }
                }
            }
            memset(&nh_arp_entry, 0, sizeof(nh_arp_entry));
            memcpy((void *)&nh_arp_entry.ip6_addr, (void *)next_hop, sizeof(nh_arp_entry.ip6_addr));
            nh_arp_entry.flags = FAL_IP_IP6_ADDR;
            rv = IP_HOST_GET(0, FAL_IP_ENTRY_IPADDR_EN, &nh_arp_entry);
            if (rv != SW_OK)
            {
                /* ARP alread setup. */
                return SW_OK;
            }
            pppoev6_sid_table.session_id = ppp_sid2;
            pppoev6_sid_table.multi_session = 1;
            pppoev6_sid_table.uni_session = 1;
            pppoev6_sid_table.entry_id = 0;
            /* set the PPPoE edit reg (0x2200), and PPPoE session reg (0x5f000) */
            rv = PPPOE_SESSION_TABLE_ADD(0, &pppoev6_sid_table);
            if (rv == SW_OK)
            {
                a_int32_t a_entry_id = -1;

                PPPOE_SESSION_ID_SET(0, pppoev6_sid_table.entry_id, pppoev6_sid_table.session_id);
                aos_printk("pppoe session: %d, entry_id: %d\n",
                           pppoev6_sid_table.session_id, pppoev6_sid_table.entry_id);
                /* create the peer host ARP entry */
                a_entry_id = arp_hw_add(S17_WAN_PORT, wan_fid, (void *)next_hop, ppp_peer_mac2, 1);
                if (a_entry_id >= 0) /* hostentry creation okay */
                {
                    rv = IP_HOST_PPPOE_BIND(0, a_entry_id, pppoev6_sid_table.entry_id, A_TRUE);
                    if ( rv != SW_OK)
                    {
                        aos_printk("IP_HOST_PPPOE_BIND failed (entry: %d, rv: %d)... \n",
                                   a_entry_id, rv);
                        PPPOE_SESSION_TABLE_DEL(0, &pppoev6_sid_table);
                        return SW_FAIL;
                    }
                    droute_entry_id = a_entry_id;
                }
                else
                {
                    PPPOE_SESSION_TABLE_DEL(0, &pppoev6_sid_table);
                    return SW_FAIL;
                }
            }
            else
            {
                aos_printk("PPPoE session add failed.. (id: %d)\n",
                           pppoev6_sid_table.session_id);
                aos_printk("rv: %d\n", rv);
                return SW_FAIL;
            }
            ipv6_droute_add_acl_rules(&local_lan6ip, droute_entry_id);
        }
    }

    return SW_OK;
}

uint32_t napt_set_ipv6_default_route(void)
{
    sw_error_t rv;
    static a_bool_t ipv6_droute_setup = A_FALSE;
    static struct ipv6_default_route_binding ipv6_droute_bind = {IN6ADDR_ANY_INIT,0};
    struct in6_addr local_lan6ip = IN6ADDR_ANY_INIT;
    unsigned long  flags;

    /* search for the next hop (s)*/
    if (NF_S17_WAN_TYPE_IP == nf_athrs17_hnat_wan_type)
    {
        struct  in6_addr *next_hop = get_ipv6_default_gateway();

        // printk("IPv6 next hop: %pI6\n", next_hop);

        if (next_hop != NULL)
        {
            fal_host_entry_t ipv6_neigh_entry;

            if (__ipv6_addr_type(next_hop) == IPV6_ADDR_LINKLOCAL)
                return SW_OK;

            local_irq_save(flags);
            memcpy(&local_lan6ip, &lan6ip, sizeof(struct in6_addr));
            local_irq_restore(flags);

            memset(&ipv6_neigh_entry, 0, sizeof(ipv6_neigh_entry));
            memcpy(&ipv6_neigh_entry.ip6_addr, next_hop, 16);
            ipv6_neigh_entry.flags = FAL_IP_IP6_ADDR;
            rv = IP_HOST_GET(0, FAL_IP_ENTRY_IPADDR_EN, &ipv6_neigh_entry);
            if ((rv != SW_OK)||(__ipv6_addr_type(&local_lan6ip) == IPV6_ADDR_ANY))
            {
                if (ipv6_droute_setup)
                {
                    ipv6_droute_del_acl_rules();
                    memset(&ipv6_droute_bind, 0, sizeof(ipv6_droute_bind));
                    ipv6_droute_setup = A_FALSE;
                }
            }
            else
            {
                if (ipv6_droute_setup)
                {
                    if (!ipv6_addr_equal(&ipv6_droute_bind.next_hop, next_hop) ||
                            ipv6_droute_bind.nh_entry_id != ipv6_neigh_entry.entry_id)
                    {
                        ipv6_droute_del_acl_rules();
                    }
                }
                ipv6_droute_bind.next_hop = *next_hop;
                ipv6_droute_bind.nh_entry_id = ipv6_neigh_entry.entry_id;

                ipv6_droute_add_acl_rules(&local_lan6ip, ipv6_neigh_entry.entry_id);
                ipv6_droute_setup = A_TRUE;
            }
        }
        else
        {
            if (ipv6_droute_setup)
            {
                ipv6_droute_del_acl_rules();
                memset(&ipv6_droute_bind, 0, sizeof(ipv6_droute_bind));
                ipv6_droute_setup = A_FALSE;
            }
        }
    }
    else if (NF_S17_WAN_TYPE_IP == nf_athrs17_hnat_wan_type)
    {
        add_pppoev6_host_entry();
    }

    return SW_OK;
}
#endif /* ifdef CONFIG_IPV6_HWACCEL */

static sw_error_t setup_interface_entry(char *list_if, int is_wan)
{
    char temp[IFNAMSIZ*4]; /* Max 4 interface entries right now. */
    char *dev_name, *list_all;
    struct net_device *nat_dev;
    struct in_device *in_device_lan = NULL;
    uint8_t *devmac, if_mac_addr[MAC_LEN];
    char *br_name;
    uint32_t vid = 0;
    sw_error_t setup_error;
    uint32_t ipv6 = 0;
    static int setup_lan_if = 0;

    memcpy(temp, list_if, strlen(list_if)+1);
    list_all = temp;

    setup_error = SW_FAIL;
    while ((dev_name = strsep(&list_all, " ")) != NULL)
    {
        nat_dev = dev_get_by_name(&init_net, dev_name);
        if (NULL == nat_dev)
        {
            // printk("%s: Cannot get device %s by name!\n", __FUNCTION__, dev_name);
            setup_error = SW_FAIL;
            continue;
        }
#if defined (CONFIG_BRIDGE)
#ifdef KVER32
        if (NULL != br_port_get_rcu(nat_dev)) /* under bridge interface. */
        {
            /* Get bridge interface name */
            br_name = (char *)(br_port_get_rcu(nat_dev)->br->dev->name);
            memcpy (nat_bridge_dev, br_name, sizeof(br_name));
            /* Get dmac */
            devmac = (uint8_t *)(br_port_get_rcu(nat_dev)->br->dev->dev_addr);
        }
#else
        if (NULL != nat_dev->br_port) /* under bridge interface. */
        {
            /* Get bridge interface name */
            br_name = (char *)nat_dev->br_port->br->dev->name;
            memcpy (nat_bridge_dev, br_name, sizeof(br_name));
            /* Get dmac */
            devmac = (uint8_t *)nat_dev->br_port->br->dev->dev_addr;
        }
#endif
        else
#endif /* CONFIG_BRIDGE */
        {
            devmac = (uint8_t *)nat_dev->dev_addr;
        }
        /* get vid */
#if defined(CONFIG_VLAN_8021Q) || defined(CONFIG_VLAN_8021Q_MODULE)
        vid = vlan_dev_vlan_id(nat_dev);
#else
        vid = 0;
#endif
#ifdef CONFIG_IPV6_HWACCEL
        ipv6 = 1;
        if (is_wan)
        {
            wan_fid = vid;
        }
#else
        ipv6 = 0;
        if (is_wan)
        {
            if (NF_S17_WAN_TYPE_PPPOEV6 == nf_athrs17_hnat_wan_type)
                ipv6 = 1;
            wan_fid = vid;
        }
#endif
#ifdef ISISC
        if (0 == is_wan) /* Not WAN -> LAN */
        {
            /* Setup private and netmask as soon as possible */
            if (br_port_get_rcu(nat_dev)) /* under bridge interface. */
            {
                in_device_lan = (struct in_device *) (br_port_get_rcu(nat_dev)->br->dev->ip_ptr);
            }
            else
            {
                in_device_lan = (struct in_device *) nat_dev->ip_ptr;
            }

            if ((in_device_lan) && (in_device_lan->ifa_list))
            {
            nat_hw_prv_mask_set((a_uint32_t)(in_device_lan->ifa_list->ifa_mask));
            nat_hw_prv_base_set((a_uint32_t)(in_device_lan->ifa_list->ifa_address));
#ifndef KVER32
            printk("Set private base 0x%08x for %s\n", (a_uint32_t)(in_device_lan->ifa_list->ifa_address), nat_dev->br_port->br->dev->name);
#endif
            memcpy(&lanip, (void *)&(in_device_lan->ifa_list->ifa_address), 4); /* copy Lan port IP. */
#ifndef ISISC
            redirect_internal_ip_packets_to_cpu_on_wan_add_acl_rules((a_uint32_t)(in_device_lan->ifa_list->ifa_address),
                                                                        (a_uint32_t)(in_device_lan->ifa_list->ifa_mask));
#endif
            }

            if(setup_lan_if) {
                return SW_OK;
            } else {
                setup_lan_if = 1;
                printk("Setup LAN interface entry!\n");
            }
        }
#endif
        memcpy(if_mac_addr, devmac, MAC_LEN);
        devmac = if_mac_addr;
        dev_put(nat_dev);

        HNAT_PRINTK("DMAC: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",
                    devmac[0], devmac[1], devmac[2],
                    devmac[3], devmac[4], devmac[5]);
        HNAT_PRINTK("VLAN id: %d\n", vid);

        if(if_mac_add(devmac, vid, ipv6) != 0)
        {
            setup_error = SW_FAIL;
            continue;
        }
        else
        {
            setup_error = SW_OK;
        }
    }

    return setup_error;
}

static int setup_all_interface_entry(void)
{
    static int setup_wan_if = 0;
    //static int setup_lan_if=0;
    static int setup_default_vid = 0;
    int i = 0;

    if (0 == setup_default_vid)
    {
        for (i=0; i<7; i++) /* For AR8327/AR8337, only 7 port */
        {
#if NAT_TODO /* need to implement here */
            PORTVLAN_ROUTE_DEFV_SET(0, i);
#endif
        }
        setup_default_vid = 1;
    }

    //if (0 == setup_lan_if)
    {
#ifdef ISISC
        //MISC_ARP_CMD_SET(0, FAL_MAC_FRWRD);
#if 0
        MISC_ARP_SP_NOT_FOUND_SET(0, FAL_MAC_RDT_TO_CPU);
#endif
#endif
        if (SW_OK == setup_interface_entry(nat_lan_dev_list, 0))
        {
            //setup_lan_if = 1; /* setup LAN interface entry success */
            //printk("Setup LAN interface entry done!\n");
        }
    }

    if (0 == setup_wan_if)
    {
        if (SW_OK == setup_interface_entry(nat_wan_dev_list, 1))
        {
            setup_wan_if = 1; /* setup WAN interface entry success */
            printk("Setup WAN interface entry done!\n");
        }
    }
#ifndef ISISC /* For S17c only */
    if ((nf_athrs17_hnat_wan_type == NF_S17_WAN_TYPE_PPPOE) ||
            (nf_athrs17_hnat_wan_type == NF_S17_WAN_TYPE_PPPOEV6))
    {
        uint8_t buf[6];

        memcpy(buf, nf_athrs17_hnat_ppp_peer_mac, ETH_ALEN);
        HNAT_PRINTK("Peer MAC: %s ", buf);
        /* add the peer interface with VID */
        if_mac_add(buf, wan_fid, 0);
        HNAT_PRINTK(" --> (%.2x-%.2x-%.2x-%.2x-%.2x-%.2x)\n", \
                    buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
        memcpy(&wanip, (void *)&nf_athrs17_hnat_wan_ip, 4);
    }
#endif

    return 1;
}

/* check for pppoe session change */
static void isis_pppoe_check_for_redial(void)
{
    if (nf_athrs17_hnat_wan_type == NF_S17_WAN_TYPE_IP)
        return;

    if(((nf_athrs17_hnat_wan_type == NF_S17_WAN_TYPE_PPPOE) \
            || (nf_athrs17_hnat_wan_type == NF_S17_WAN_TYPE_PPPOEV6)) \
            && (pppoetbl.session_id != 0))
    {
        if(pppoetbl.session_id != nf_athrs17_hnat_ppp_id)
        {
            aos_printk("%s: PPPoE session ID changed... \n", __func__);
            if (nf_athrs17_hnat_wan_type != NF_S17_WAN_TYPE_PPPOEV6)
            {
                if (PPPOE_SESSION_TABLE_DEL(0, &pppoetbl) != SW_OK)
                {
                    aos_printk("delete old pppoe session %d entry_id %d failed.. \n", pppoetbl.session_id, pppoetbl.entry_id);
                    return;
                }

                /* force PPPoE parser for multi- and uni-cast packets; for v1.0.7+ */
                pppoetbl.session_id = nf_athrs17_hnat_ppp_id;
                pppoetbl.multi_session = 1;
                pppoetbl.uni_session = 1;
                pppoetbl.entry_id = 0;
                /* set the PPPoE edit reg (0x2200), and PPPoE session reg (0x5f000) */
                if (PPPOE_SESSION_TABLE_ADD(0, &pppoetbl) == SW_OK)
                {
                    PPPOE_SESSION_ID_SET(0, pppoetbl.entry_id, pppoetbl.session_id);
                    printk("%s: new pppoe session id: %x, entry_id: %x\n", __func__, pppoetbl.session_id, pppoetbl.entry_id);
                }
            }
            else  /* nf_athrs17_hnat_wan_type == NF_S17_WAN_TYPE_PPPOEV6 */
            {
                /* reset the session Id only */
                aos_printk("IPV6 PPPOE mode... \n");
                pppoetbl.session_id = nf_athrs17_hnat_ppp_id;
                PPPOE_SESSION_ID_SET(0, pppoetbl.entry_id, pppoetbl.session_id);
                printk("%s: new pppoe session id: %x, entry_id: %x\n", __func__, pppoetbl.session_id, pppoetbl.entry_id);
            }
            /* read back the WAN IP */
            memcpy(&wanip, (void *)&nf_athrs17_hnat_wan_ip, 4);
            aos_printk("Read the WAN IP back... %.8x\n", *(uint32_t *)&wanip);
            /* change the PPPoE ACL to ensure the packet is correctly forwarded by the HNAT engine */
            pppoe_add_acl_rules(*(uint32_t *)&wanip, *(uint32_t *)&lanip, pppoe_gwid);
        }
    }
}

#ifdef ISIS /* only for S17 */
static void pppoev6_mac6_loop_dev(void)
{
#define PPPOEV6_SESSION_ID  0xfffe
    fal_pppoe_session_t ptbl;

    memset(&ptbl, 0, sizeof(fal_pppoe_session_t));

    aos_printk("%s: set MAC6 as loopback device\n", __func__);

    ptbl.session_id = PPPOEV6_SESSION_ID;
    ptbl.multi_session = 1;
    ptbl.uni_session = 1;
    ptbl.entry_id = 0xe;

    /* set the PPPoE edit reg (0x2200), and PPPoE session reg (0x5f000) */
    if (PPPOE_SESSION_TABLE_ADD(0, &ptbl) == SW_OK)
    {
        PPPOE_SESSION_ID_SET(0, ptbl.entry_id, ptbl.session_id);
        aos_printk("%s: pppoe session id: %d added into entry: %d \n", __func__, ptbl.session_id, ptbl.entry_id);
    }
    else
    {
        aos_printk("%s: failed on adding pppoe session id: %d\n", __func__, ptbl.session_id);
    }

    /* PPPoE entry 0 */
    athrs17_reg_write(0x2200, PPPOEV6_SESSION_ID);

    aos_printk("%s: end of function... \n", __func__);
}

static void pppoev6_remove_parser(uint32_t entry_id)
{
    aos_printk("%s: clear entry id: %d\n", __func__, entry_id);
    /* clear the session id in the PPPoE parser engine */
    athrs17_reg_write(PPPOE_SESSION_OFFSET + PPPOE_SESSION_E_OFFSET * entry_id, 0);
}

static void pppoev6_mac6_stop_learning(void)
{
    /* do not disable this port if some other registers are already filled in
       to prevent setting conflict */
    int val = S17_P6PAD_MODE_REG_VALUE;

    if ( val != (1<<24))
    {
        aos_printk("%s: MAC 6 already being used!\n", __FUNCTION__);
        return;
    }


    /* clear the MAC6 learning bit */
    athrs17_reg_write(0x6a8, athrs17_reg_read(0x6a8) & ~(1<<20));

    /* force loopback mode */
    athrs17_reg_write(0x94, 0x7e);
    athrs17_reg_write(0xb4, 0x10);
}
#endif // ifdef ISIS

static int add_pppoe_host_entry(uint32_t sport, a_int32_t arp_entry_id)
{
    a_bool_t ena;
    int rv = SW_OK;
    fal_host_entry_t nh_arp_entry;

    if (0xffff == wan_fid)
    {
        printk("%s: Cannot get WAN vid!\n", __FUNCTION__);
        return SW_FAIL;
    }

    if (PPPOE_STATUS_GET(0, &ena) != SW_OK)
    {
        aos_printk("Cannot get the PPPoE mode\n");
        ena = 0;
    }
#ifdef ISIS
    if (!ena)
#else /* For S17c only */
    memset(&nh_arp_entry, 0, sizeof(nh_arp_entry));
    nh_arp_entry.ip4_addr = nf_athrs17_hnat_ppp_peer_ip;
    nh_arp_entry.flags = FAL_IP_IP4_ADDR;
    rv = IP_HOST_GET(0, FAL_IP_ENTRY_IPADDR_EN, &nh_arp_entry);
    if (SW_OK != rv)
#endif
    {
        if ((!ena) && (PPPOE_STATUS_SET(0, A_TRUE) != SW_OK))
            aos_printk("Cannot enable the PPPoE mode\n");

        aos_printk("PPPoE enable mode: %d\n", ena);

        pppoetbl.session_id = nf_athrs17_hnat_ppp_id;
        pppoetbl.multi_session = 1;
        pppoetbl.uni_session = 1;
        pppoetbl.entry_id = 0;

        /* set the PPPoE edit reg (0x2200), and PPPoE session reg (0x5f000) */
        rv = PPPOE_SESSION_TABLE_ADD(0, &pppoetbl);
        if (rv == SW_OK)
        {
            uint8_t mbuf[6], ibuf[4];
            a_int32_t a_entry_id = -1;

            PPPOE_SESSION_ID_SET(0, pppoetbl.entry_id, pppoetbl.session_id);
            aos_printk("pppoe session: %d, entry_id: %d\n", pppoetbl.session_id, pppoetbl.entry_id);

            /* create the peer host ARP entry */
            memcpy(ibuf, (void *)&nf_athrs17_hnat_ppp_peer_ip, 4);
            memcpy(mbuf, nf_athrs17_hnat_ppp_peer_mac, ETH_ALEN);

            a_entry_id = arp_hw_add(S17_WAN_PORT, wan_fid, ibuf, mbuf, 0);
            if (a_entry_id >= 0) /* hostentry creation okay */
            {
                aos_printk("(1)Bind PPPoE session ID: %d, entry_id: %d to host entry: %d\n", \
                           pppoetbl.session_id, pppoetbl.entry_id, a_entry_id);

                rv = IP_HOST_PPPOE_BIND(0, a_entry_id, pppoetbl.entry_id, A_TRUE);
                if ( rv != SW_OK)
                {
                    aos_printk("IP_HOST_PPPOE_BIND failed (entry: %d, rv: %d)... \n", a_entry_id, rv);
                }

                aos_printk("adding ACLs \n");
                pppoe_gwid = a_entry_id;
                pppoe_add_acl_rules(*(uint32_t *)&wanip, *(uint32_t *)&lanip, a_entry_id);
                aos_printk("ACL creation okay... \n");
            }
        }
        else
        {
            aos_printk("PPPoE session add failed.. (id: %d)\n", pppoetbl.session_id);
            aos_printk("rv: %d\n", rv);
        }

#ifdef ISIS
        if (nf_athrs17_hnat_wan_type == NF_S17_WAN_TYPE_PPPOEV6)
        {
            aos_printk("IPV6 PPPOE mode... (share the same ID with IPV4's)\n");
            pppoev6_mac6_loop_dev();
            pppoev6_remove_parser(pppoetbl.entry_id);

            /* bind the first LAN host to the pseudo PPPoE ID */
            rv = IP_HOST_PPPOE_BIND(0, arp_entry_id, 0, A_TRUE);
            if ( rv != SW_OK)
            {
                aos_printk("IP_HOST_PPPOE_BIND failed (entry: %d, rv: %d)... \n", arp_entry_id, rv);
            }
        }
#endif // ifdef ISIS
    }
#ifdef ISIS
    else  /* ena */
    {
        if ((nf_athrs17_hnat_wan_type == NF_S17_WAN_TYPE_PPPOEV6) &&
                (sport != S17_WAN_PORT)&& (arp_entry_id != 0))
        {
            aos_printk("IPV6 PPPoE mode\n");
            /* bind LAN hosts to the pseudo PPPoE ID */
            rv = IP_HOST_PPPOE_BIND(0, arp_entry_id, 0, A_TRUE);
            if ( rv != SW_OK)
            {
                aos_printk("IP_HOST_PPPOE_BIND failed (entry: %d, rv: %d)... \n", arp_entry_id, rv);
            }
        }
    }
#endif // ifdef ISIS

    return SW_OK;
}

static int
arp_is_reply(struct sk_buff *skb)
{
    struct arphdr *arp = arp_hdr(skb);

    if (!arp)
    {
        HNAT_PRINTK("%s: Packet has no ARP data\n", __func__);
        return 0;
    }

    if (skb->len < sizeof(struct arphdr))
    {
        HNAT_PRINTK("%s: Packet is too small to be an ARP\n", __func__);
        return 0;
    }

    if (arp->ar_op != htons(ARPOP_REPLY))
    {
        return 0;
    }

    return 1;
}

static int
dev_check(char *in_dev, char *dev_list)
{
    char *list_dev;
    char temp[100] = {0};
    char *list;

    if(!in_dev || !dev_list)
    {
        return 0;
    }

    strcpy(temp, dev_list);
    list = temp;

    HNAT_PRINTK("%s: list:%s\n", __func__, list);
    while ((list_dev = strsep(&list, " ")) != NULL)
    {
        HNAT_PRINTK("%s: strlen:%d list_dev:%s in_dev:%s\n",
                    __func__, strlen(list_dev), list_dev, in_dev);

        if (!strncmp(list_dev, in_dev, strlen(list_dev)))
        {
            HNAT_PRINTK("%s: %s\n", __FUNCTION__, list_dev);
            return 1;
        }
    }

    return 0;
}

#ifndef ISISC
static uint32_t get_netmask_from_netdevice(const struct net_device *in_net_dev)
{
    struct in_device *my_in_device = NULL;
    uint32_t result = 0xffffff00;

    if((in_net_dev) && (in_net_dev->ip_ptr != NULL))
    {
        my_in_device = (struct in_device *)(in_net_dev->ip_ptr);
        if(my_in_device->ifa_list != NULL)
        {
            result = my_in_device->ifa_list->ifa_mask;
        }
    }

    return result;
}
#endif

static unsigned int
arp_in(unsigned int hook,
       struct sk_buff *skb,
       const struct net_device *in,
       const struct net_device *out,
       int (*okfn) (struct sk_buff *))
{
    struct arphdr *arp = NULL;
    uint8_t *sip, *dip, *smac, *dmac;
    uint8_t dev_is_lan = 0;
    uint32_t sport = 0, vid = 0;
#ifdef ISIS
    uint32_t lan_netmask = 0;
    a_bool_t prvbasemode = 1;
#endif
    a_int32_t arp_entry_id = -1;

    /* check for PPPoE redial here, to reduce overheads */
    isis_pppoe_check_for_redial();

    /* do not write out host table if HNAT is disabled */
    if (!nf_athrs17_hnat)
        return NF_ACCEPT;

    setup_all_interface_entry();

    if(dev_check((char *)in->name, (char *)nat_wan_dev_list))
    {

    }
    else if (dev_check((char *)in->name, (char *)nat_bridge_dev))
    {
        dev_is_lan = 1;
    }
    else
    {
        HNAT_PRINTK("Not Support device: %s\n",  (char *)in->name);
        return NF_ACCEPT;
    }

    if(!arp_is_reply(skb))
    {
        return NF_ACCEPT;
    }
#ifdef AP136_QCA_HEADER_EN
    if(arp_if_info_get((void *)(skb->head), &sport, &vid) != 0)
    {
        printk("Cannot get header info!!\n");
        return NF_ACCEPT;
    }
#else
    if(dev_is_lan) {
         vid = NAT_LAN_DEV_VID;
    } else {
         vid = NAT_WAN_DEV_VID;
    }

    fal_fdb_entry_t entry = {0};

    entry.fid = vid;
    smac  = skb->mac_header + MAC_LEN;
    aos_mem_copy(&(entry.addr), smac, sizeof(fal_mac_addr_t));

    if(fal_fdb_find(0, &entry) == SW_OK) {
        vid  = entry.fid;
        sport = 0;
        while (sport < 32) {
            if(entry.port.map & (1 << sport)) {
                break;
            }
            sport++;
        }
    } else {
        printk("not find the FDB entry\n");
    }
#endif

    arp = arp_hdr(skb);
    smac = ((uint8_t *) arp) + ARP_HEADER_LEN;
    sip = smac + MAC_LEN;
    dmac = sip + IP_LEN;
    dip = dmac + MAC_LEN;

    arp_entry_id = arp_hw_add(sport, vid, sip, smac, 0);
    if(arp_entry_id < 0)
    {
        printk("ARP entry error!!\n");
        return NF_ACCEPT;
    }

    if (0 == dev_is_lan)
    {
        memcpy(&wanip, dip, 4);
#ifdef MULTIROUTE_WR
        wan_nh_add(sip, smac, arp_entry_id);
#endif
    }

    if(dev_is_lan && nat_hw_prv_base_can_update())
    {
        nat_hw_flush();
        nat_hw_prv_base_update_disable();
#ifdef MULTIROUTE_WR
        //multi_route_indev = in;
#endif
    }
    multi_route_indev = in;

    if ((nf_athrs17_hnat_wan_type == NF_S17_WAN_TYPE_PPPOE) ||
            (nf_athrs17_hnat_wan_type == NF_S17_WAN_TYPE_PPPOEV6))
    {
        add_pppoe_host_entry(sport, arp_entry_id);
    }

#ifdef ISIS
    /* check for SIP and DIP range */
    if ((lanip[0] != 0) && (wanip[0] != 0))
    {
        if (NAT_PRV_ADDR_MODE_GET(0, &prvbasemode) != SW_OK)
        {
            aos_printk("Private IP base mode check failed: %d\n", prvbasemode);
        }

        if (!prvbasemode) /* mode 0 */
        {
            if ((lanip[0] == wanip[0]) && (lanip[1] == wanip[1]))
            {
                if ((lanip[2] & 0xf0) == (wanip[2] & 0xf0))
                {
                    if (get_aclrulemask()& (1 << S17_ACL_LIST_IPCONF))
                        return NF_ACCEPT;

                    aos_printk("LAN IP and WAN IP conflict... \n");
                    /* set h/w acl to filter out this case */
#ifdef MULTIROUTE_WR
                    // if ( (wan_nh_ent[0].host_ip != 0) && (wan_nh_ent[0].entry_id != 0))
                    if ( (wan_nh_ent[0].host_ip != 0))
                        ip_conflict_add_acl_rules(*(uint32_t *)&wanip, *(uint32_t *)&lanip, wan_nh_ent[0].entry_id);
#endif
                    return NF_ACCEPT;
                }
            }
        }
        else  /* mode 1*/
        {
            ;; /* do nothing */
        }
    }
#endif /* ifdef ISIS */

    return NF_ACCEPT;
}

static struct
        nf_hook_ops arpinhook =
{
    .hook = arp_in,
    .hooknum = NF_ARP_IN,
    .owner = THIS_MODULE,
    .pf = NFPROTO_ARP,
    .priority = NF_IP_PRI_FILTER,
};

#ifdef AUTO_UPDATE_PPPOE_INFO
static int qcaswitch_pppoe_ip_event(struct notifier_block *this,
                                    unsigned long event, void *ptr)
{
    struct in_ifaddr *ifa = (struct in_ifaddr *)ptr;
    struct net_device *dev = (struct net_device *)ifa->ifa_dev->dev;
    struct ppp *ppp = netdev_priv(dev);
    struct list_head *list;
    struct channel *pch;
    struct sock *sk;
    struct pppox_sock *po;
    unsigned long  flags;
    static int connection_count = 0;
    fal_pppoe_session_t del_pppoetbl;

    if (!((dev->type == ARPHRD_PPP) && (dev->flags & IFF_POINTOPOINT)))
        return NOTIFY_DONE;

    if (dev_net(dev) != &init_net)
        return NOTIFY_DONE;

    setup_all_interface_entry();

    switch (event)
    {
        case NETDEV_UP:
            local_irq_save(flags);
            list = &ppp->channels;
            if (list_empty(list))
            {
                local_irq_restore(flags);
                return NOTIFY_DONE;
            }
            if ((ppp->flags & SC_MULTILINK) == 0)
            {
                /* not doing multilink: send it down the first channel */
                list = list->next;
                pch = list_entry(list, struct channel, clist);
                if (pch->chan)
                {
                    if (pch->chan->private)
                    {
                        /* the NETDEV_UP event will be sent many times
                        * because of ifa operation ifa->ifa_local != ifa->ifa_address
                        * means that remote ip is really added.
                        */
                        if (ifa->ifa_local == ifa->ifa_address)
                        {
                            local_irq_restore(flags);
                            return NOTIFY_DONE;
                        }
                        sk = (struct sock *)pch->chan->private;
                        po = (struct pppox_sock*)sk;
                        connection_count++;
                        if (((NF_S17_WAN_TYPE_PPPOE == nf_athrs17_hnat_wan_type) &&
                                (0 != nf_athrs17_hnat_ppp_id))) /* another session for IPv6 */
                        {
                            nf_athrs17_hnat_ppp_id2 = po->num;
                            memcpy(nf_athrs17_hnat_ppp_peer_mac2, po->pppoe_pa.remote, ETH_ALEN);
                        }
                        else
                        {
                            nf_athrs17_hnat_wan_type = NF_S17_WAN_TYPE_PPPOE;
                            nf_athrs17_hnat_wan_ip = ifa->ifa_local;
                            nf_athrs17_hnat_ppp_peer_ip = ifa->ifa_address;
                            memcpy(nf_athrs17_hnat_ppp_peer_mac, po->pppoe_pa.remote, ETH_ALEN);
                            nf_athrs17_hnat_ppp_id = po->num;
                        }
                    }
                }
                else
                {
                    local_irq_restore(flags);
                    /* channel got unregistered */
                    return NOTIFY_DONE;
                }
            }
            local_irq_restore(flags);
            break;

        case NETDEV_DOWN:
            if (NF_S17_WAN_TYPE_PPPOE != nf_athrs17_hnat_wan_type)
            {
                return NOTIFY_DONE;
            }
            printk("DOWN: local: "NIPQUAD_FMT"\n", NIPQUAD(ifa->ifa_local));
            printk("DOWN: address: "NIPQUAD_FMT"\n", NIPQUAD(ifa->ifa_address));
            connection_count--;
            local_irq_save(flags);
            if (ifa->ifa_local == nf_athrs17_hnat_wan_ip)
            {   
                /* PPPoE Interface really down */
                ipv6_droute_del_acl_rules();
                del_pppoetbl.session_id = nf_athrs17_hnat_ppp_id;
                del_pppoetbl.multi_session = 1;
                del_pppoetbl.uni_session = 1;
                del_pppoetbl.entry_id = 0;
                PPPOE_SESSION_TABLE_DEL(0, &del_pppoetbl);
                nf_athrs17_hnat_wan_type = NF_S17_WAN_TYPE_IP;
                nf_athrs17_hnat_wan_ip = 0;
                nf_athrs17_hnat_ppp_peer_ip = 0;
                nf_athrs17_hnat_ppp_id = 0;
                memset(&nf_athrs17_hnat_ppp_peer_mac, 0, ETH_ALEN);
            }
            else
            {
                if (0 != nf_athrs17_hnat_ppp_id2)
                {
                    del_pppoetbl.session_id = nf_athrs17_hnat_ppp_id2;
                    del_pppoetbl.multi_session = 1;
                    del_pppoetbl.uni_session = 1;
                    del_pppoetbl.entry_id = 0;
                    PPPOE_SESSION_TABLE_DEL(0, &del_pppoetbl);
                }
                nf_athrs17_hnat_ppp_id2 = 0;
                memset(&nf_athrs17_hnat_ppp_peer_mac2, 0, ETH_ALEN);
            }
            qcaswitch_hostentry_flush();
            local_irq_restore(flags);
            break;

        default:
            break;
    }
    return NOTIFY_DONE;
}

/* a linux interface is configured with ipaddr, then
 * it becomes a L3 routing interface
 * add the router mac of this interface to the table
 */
/* FIXME: only hande pppoe event right now. */
static int qcaswitch_ip_event(struct notifier_block *this,
                              unsigned long event, void *ptr)
{
    struct in_ifaddr *ifa = (struct in_ifaddr *)ptr;
    struct net_device *dev = (struct net_device *)ifa->ifa_dev->dev;

    if ((dev->type == ARPHRD_PPP) && (dev->flags & IFF_POINTOPOINT))
    {
        return qcaswitch_pppoe_ip_event(this, event, ptr);
    }

    return NOTIFY_DONE;
}


static struct notifier_block qcaswitch_ip_notifier =
{
    .notifier_call = qcaswitch_ip_event,
    .priority = 100,
};
#endif // ifdef AUTO_UPDATE_PPPOE_INFO

#define HOST_AGEOUT_STATUS 1
void host_check_aging(void)
{
    fal_host_entry_t *host_entry_p, host_entry= {0};
    sw_error_t rv;
    int cnt = 0;
    unsigned long flags;
    fal_napt_entry_t src_napt = {0}, pub_napt = {0};

    host_entry_p = &host_entry;
    host_entry_p->entry_id = FAL_NEXT_ENTRY_FIRST_ID;

    local_irq_save(flags);
    while (1)
    {
        host_entry_p->status = HOST_AGEOUT_STATUS;
        /* FIXME: now device id is set to 0. */
        rv = IP_HOST_NEXT (0, FAL_IP_ENTRY_STATUS_EN, host_entry_p);
        // rv = IP_HOST_NEXT (0, 0, host_entry_p);
        if (SW_OK != rv)
            break;
        if (cnt >= ARP_ENTRY_MAX) // arp entry number
            break;

        if (ARP_AGE_NEVER == host_entry_p->status)
            continue;

        if ((S17_WAN_PORT == host_entry_p->port_id) &&
                (host_entry_p->counter_en))
        {
            if (0 != host_entry_p->packet)
            {
                // arp entry is using, update it.
                host_entry.status = ARP_AGE;
                printk("Update WAN port hostentry!\n");
                IP_HOST_ADD(0, host_entry_p);
            }
            else
            {
                printk("Del WAN port hostentry!\n");
                IP_HOST_DEL(0, FAL_IP_ENTRY_IPADDR_EN, host_entry_p);
            }
            continue;
        }

        src_napt.entry_id = FAL_NEXT_ENTRY_FIRST_ID;
        memcpy(&src_napt.src_addr, &host_entry_p->ip4_addr, sizeof(fal_ip4_addr_t));
        pub_napt.entry_id = FAL_NEXT_ENTRY_FIRST_ID;
        memcpy(&pub_napt.trans_addr, &host_entry_p->ip4_addr, sizeof(fal_ip4_addr_t));
        if((NAPT_NEXT(0, FAL_NAT_ENTRY_SOURCE_IP_EN ,&src_napt) !=0) && \
                (NAPT_NEXT(0, FAL_NAT_ENTRY_PUBLIC_IP_EN ,&pub_napt) != 0))
        {
            /* Cannot find naptentry */
            printk("ARP id 0x%x: Cannot find NAPT entry!\n", host_entry_p->entry_id);
            IP_HOST_DEL(0, FAL_IP_ENTRY_IPADDR_EN, host_entry_p);
            continue;
        }
        // arp entry is using, update it.
        host_entry_p->status = ARP_AGE;
        IP_HOST_ADD(0, host_entry_p);
        printk("update entry 0x%x port %d\n", host_entry_p->entry_id, host_entry_p->port_id);
        cnt++;
    }
    local_irq_restore(flags);
}

#ifdef CONFIG_IPV6_HWACCEL
#define IPV6_LEN 16
#define MAC_LEN 6
#define PROTO_ICMPV6 0x3a
#define NEIGHBOUR_SOL 135
#define NEIGHBOUR_AD 136

struct icmpv6_option
{
    __u8 type;
    __u8 len;
    __u8 mac[MAC_LEN];
};

static unsigned int ipv6_handle(unsigned   int   hooknum,
                                struct   sk_buff   *skb,
                                const   struct   net_device   *in,
                                const   struct   net_device   *out,
                                int   (*okfn)(struct   sk_buff   *))
{
    struct ipv6hdr *iph6 = ipv6_hdr(skb);
    struct icmp6hdr *icmp6 = icmp6_hdr(skb);
    __u8 *sip = ((__u8 *)icmp6)+sizeof(struct icmp6hdr);
    struct icmpv6_option *icmpv6_opt = (struct icmpv6_option *)(sip+IPV6_LEN);
    __u8 *sa = icmpv6_opt->mac;

    uint32_t sport = 0, vid = 0;
    struct inet6_ifaddr *in_device_addr = NULL;
    uint8_t dev_is_lan = 0;
    uint8_t *smac;

    /* do not write out host table if HNAT is disabled */
    if (!nf_athrs17_hnat)
        return NF_ACCEPT;

    setup_all_interface_entry();

    if(dev_check((char *)in->name, (char *)nat_wan_dev_list))
    {
        dev_is_lan = 0;
    }
    else if (dev_check((char *)in->name, (char *)nat_bridge_dev))
    {
        dev_is_lan = 1;
    }
    else
    {
        HNAT_PRINTK("Not Support device: %s\n",  (char *)in->name);
        return NF_ACCEPT;
    }

    if(PROTO_ICMPV6 == iph6->nexthdr)
    {
        if(NEIGHBOUR_AD == icmp6->icmp6_type)
        {
            if (__ipv6_addr_type((struct in6_addr*)sip) & IPV6_ADDR_LINKLOCAL)
                return NF_ACCEPT;

#ifdef AP136_QCA_HEADER_EN
            if(arp_if_info_get((void *)(skb->head), &sport, &vid) != 0)
            {
                return NF_ACCEPT;
            }

            if ((0 == vid)||(0 == sport))
            {
                printk("Error: Null sport or vid!!\n");
                return NF_ACCEPT;
            }
#else
            if(dev_is_lan) {
                 vid = NAT_LAN_DEV_VID;
            } else {
                 vid = NAT_WAN_DEV_VID;
            }

            fal_fdb_entry_t entry = {0};

            entry.fid = vid;
            smac  = skb->mac_header + MAC_LEN;
            aos_mem_copy(&(entry.addr), smac, sizeof(fal_mac_addr_t));

            if(fal_fdb_find(0, &entry) == SW_OK) {
                vid  = entry.fid;
                sport = 0;
                while (sport < 32) {
                    if(entry.port.map & (1 << sport)) {
                        break;
                    }
                    sport++;
                }
            } else {
                printk("not find the FDB entry\n");
            }
#endif
            if ((0 == dev_is_lan) && (S17_WAN_PORT != sport))
            {
                printk("Error: WAN port %d\n", sport);
                return NF_ACCEPT;
            }

            HNAT_PRINTK("ND Reply %x %x\n",icmpv6_opt->type,icmpv6_opt->len);
            HNAT_PRINTK("isis_v6: incoming packet, sip = %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x\n"
                        ,sip[0],sip[1],sip[2],sip[3],sip[4],sip[5],sip[6],sip[7]
                        ,sip[8],sip[9],sip[10],sip[11],sip[12],sip[13],sip[14],sip[15]
                       );
            HNAT_PRINTK("isis_v6: incoming packet, sa  = %.2x-%.2x-%.2x-%.2x-%.2x-%.2x\n", sa[0], sa[1], sa[2], sa[3], sa[4], sa[5]);
            HNAT_PRINTK("isis_v6: vid = %d sport = %d\n", vid, sport);

            //add nd entry
            if((2 == icmpv6_opt->type) && (1 == icmpv6_opt->len))
            {
                arp_hw_add(sport, vid, sip, sa, 1);
            }
            else /* ND AD packets without option filed? Fix Me!! */
            {
                sa = skb->mac_header + MAC_LEN;
                HNAT_PRINTK("isis_v6 Changed sa  = %.2x-%.2x-%.2x-%.2x-%.2x-%.2x\n", sa[0], sa[1], sa[2], sa[3], sa[4], sa[5]);
                arp_hw_add(sport, vid, sip, sa, 1);
            }

#if NAT_TODO /* should be ok */
            if ((NULL != in->ip6_ptr) && (NULL != ((struct inet6_dev *)in->ip6_ptr)->addr_list))
#else
            if (NULL != in->ip6_ptr)
#endif
            {
                //list_for_each_entry(in_device_addr, &(in->ip6_ptr)->addr_list, if_list);
                struct inet6_dev *idev = __in6_dev_get(in);
                list_for_each_entry(in_device_addr, &idev->addr_list, if_list) {
            		if (in_device_addr->scope == 0 &&
            		    !(in_device_addr->flags & IFA_F_TENTATIVE)) {
            			break;
            		}
        	    }

                if (0 == dev_is_lan)
                {
                    /* WAN ipv6 address*/
                    memcpy(&wan6ip, (__u8 *)&in_device_addr->addr, sizeof(struct in6_addr));
                    HNAT_PRINTK("%s: ipv6 wanip %pI6\n", in->name, &wan6ip);
                }
                else
                {
                    /* LAN ipv6 address*/
                    memcpy(&lan6ip, (__u8 *)&in_device_addr->addr, sizeof(struct in6_addr));
                    HNAT_PRINTK("%s: ipv6 lanip %pI6\n", in->name, &lan6ip);
                }
            }
        }
    }

    return NF_ACCEPT;
}

static struct nf_hook_ops ipv6_inhook =
{
    .hook = ipv6_handle,
    .owner = THIS_MODULE,
    .pf = PF_INET6,
    .hooknum = NF_INET_PRE_ROUTING,
    .priority = NF_IP6_PRI_CONNTRACK,
};
#endif /* CONFIG_IPV6_HWACCEL */

extern int napt_procfs_init(void);
extern void napt_procfs_exit(void);

void host_helper_init(void)
{
    int i;
    sw_error_t rv;
    a_uint32_t entry;

    /* header len 4 with type 0xaaaa */
    HEADER_TYPE_SET(0, A_TRUE, 0xaaaa);
#ifdef ISISC
    /* For S17c (ISISC), it is not necessary to make all frame with header */
    printk("host_helper_init start\n");
    //PORT_TXHDR_MODE_SET(0, 0, FAL_ONLY_MANAGE_FRAME_EN);
    /* Fix tag disappear problem, set TO_CPU_VID_CHG_EN, 0xc00 bit1 */
    CPU_VID_EN_SET(0, A_TRUE);
    /* set RM_RTD_PPPOE_EN, 0xc00 bit0 */
    RTD_PPPOE_EN_SET(0, A_TRUE);
    /* Enable ARP ack frame as management frame. */
    for (i=1; i<6; i++)
    {
        PORT_ARP_ACK_STATUS_SET(0, i, A_TRUE);
    }
    MISC_ARP_CMD_SET(0, FAL_MAC_FRWRD);
    /* Avoid ARP response storm for HUB, now this fix only apply on PORT5 */
#if 0
    MISC_ARP_SP_NOT_FOUND_SET(0, FAL_MAC_RDT_TO_CPU);
    MISC_ARP_GUARD_SET(0, S17_WAN_PORT, FAL_MAC_IP_PORT_GUARD);
#endif
    /* set VLAN_TRANS_TEST register bit, to block packets from WAN port has private dip */
    NETISOLATE_SET(0, A_TRUE);
#else
    PORT_TXHDR_MODE_SET(0, 0, FAL_ALL_TYPE_FRAME_EN);
#endif
    CPU_PORT_STATUS_SET(0, A_TRUE);
    IP_ROUTE_STATUS_SET(0, A_TRUE);

    /* CPU port with VLAN tag, others w/o VLAN */
    entry = 0x01111112;
    HSL_REG_ENTRY_SET(rv, 0, ROUTER_EG, 0, (a_uint8_t *) (&entry), sizeof (a_uint32_t));

    napt_procfs_init();
    memcpy(nat_bridge_dev, nat_lan_dev_list, strlen(nat_lan_dev_list)+1);

    nf_register_hook(&arpinhook);
#ifdef CONFIG_IPV6_HWACCEL
    aos_printk("Registering IPv6 hooks... \n");
    nf_register_hook(&ipv6_inhook);
#endif

#ifdef AUTO_UPDATE_PPPOE_INFO
    register_inetaddr_notifier(&qcaswitch_ip_notifier);
#endif // ifdef AUTO_UPDATE_PPPOE_INFO

    /* Enable ACLs to handle MLD packets */
    upnp_ssdp_add_acl_rules();
    ipv6_snooping_solicted_node_add_acl_rules();
    ipv6_snooping_sextuple0_group_add_acl_rules();
    ipv6_snooping_quintruple0_1_group_add_acl_rules();
}

void host_helper_exit(void)
{
    napt_procfs_exit();

    nf_unregister_hook(&arpinhook);
#ifdef CONFIG_IPV6_HWACCEL
    nf_unregister_hook(&ipv6_inhook);
#endif
}

