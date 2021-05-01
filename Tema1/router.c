#include "skel.h"
#include "data.h"
#include "queue.h"
#include <netinet/if_ether.h>

struct rtable_entry *rtable;
int rtable_size;

// arp table
struct arp_entry *arptable;
int arptable_size;

// messages to send
queue to_send;
int size_q;

int comparator(const void *p, const void *q) {
  uint32_t l = ((struct rtable_entry *)p)->prefix;
  uint32_t r = ((struct rtable_entry *)q)->prefix;
  uint32_t l2 = ((struct rtable_entry *)p)->mask;
  uint32_t r2 = ((struct rtable_entry *)q)->mask;

  // comparing prefix
  if (l != r) return (l - r);

  // comparing mask
  if (l == r) return (ntohs(l2) - ntohs(r2));
  return -1;
}

struct arp_entry *get_arp_entry(__u32 ip) {
  // searching ip address
  for (int i = 0; i < arptable_size; ++i)
    if (arptable[i].ip == ip) return &arptable[i];

  return NULL;
}

// binary search for best route
struct rtable_entry *get_best_route(uint32_t dest_ip, int l, int r) {
  int sol = -1;
  while (l <= r) {
    int m = (l + r) / 2;
    // keep ip position and keep serching for better route
    if (rtable[m].prefix <= (dest_ip & rtable[m].mask)) {
      sol = m;
      l = m + 1;
      continue;
    }
    m--;
    r = m;
  }

  if (sol < 0) return NULL;
  return &rtable[sol];
}

int main(int argc, char *argv[]) {
  packet m;
  int rc;
  init();
  int total = 100;
  rtable = malloc(sizeof(struct rtable_entry) * total);
  arptable = malloc(sizeof(struct arp_entry) * 100000);
  arptable_size = 0;
  rtable_size = parse_rtable(&rtable, total);
  to_send = queue_create();
  size_q = 0;
  qsort(rtable, rtable_size, sizeof(rtable[0]), comparator);

  // display routing table

  /*for(int i = 0; i < rtable_size; i++){
          printf("Prefix: %u Next_hop: %u Mask: %u Interface: %d\n",
  rtable[i].prefix, rtable[i].next_hop, rtable[i].mask , rtable[i].interface);
  }
  printf("Ending %d", rtable_size);
  return 0;
  */

  while (1) {
    rc = get_packet(&m);
    DIE(rc < 0, "get_message");

    /* Students will write code here */
    struct ether_header *eth_hdr = (struct ether_header *)m.payload;
    struct iphdr *ip_hdr =
        (struct iphdr *)(m.payload + sizeof(struct ether_header));



    /*received an arp packet*/
    if (ntohs(eth_hdr->ether_type) == ETHERTYPE_ARP) {
      struct ether_arp *arp_eth =
          (struct ether_arp *)(m.payload + sizeof(struct ether_header));
      /* received arp request and sending arp reply*/
      if (ntohs(arp_eth->arp_op) == ARPOP_REQUEST) {
        memcpy(eth_hdr->ether_dhost, eth_hdr->ether_shost,
               sizeof(eth_hdr->ether_shost));
        get_interface_mac(m.interface, eth_hdr->ether_shost);

        memcpy(arp_eth->arp_tha, arp_eth->arp_sha, sizeof(arp_eth->arp_sha));
        get_interface_mac(m.interface, arp_eth->arp_sha);
        arp_eth->arp_op = htons(ARPOP_REPLY);

        char aux3[6];
        memcpy(aux3, arp_eth->arp_spa, sizeof(arp_eth->arp_spa));
        memcpy(arp_eth->arp_spa, arp_eth->arp_tpa,
               sizeof(arp_eth->arp_tpa));  // arp sender protocol (ip)
        memcpy(arp_eth->arp_tpa, aux3,
               sizeof(arp_eth->arp_spa));  // arp target protocol(ip)
        send_packet(m.interface, &m);
        continue;
      }

      /* received arp reply, updating arp table and sending packets*/
      if (ntohs(arp_eth->arp_op) == ARPOP_REPLY) {
        arptable[arptable_size].ip = inet_addr(arp_eth->arp_spa);
        hwaddr_aton(arp_eth->arp_sha, arptable[arptable_size].mac);
        arptable_size++;

        // sending packets from queue
        int i = 0;
        for(i = 0; i < size_q; i++) {
          packet *x = queue_deq(to_send);
          struct ether_header *eth = (struct ether_header *)x->payload;
          get_interface_mac(x->interface, eth->ether_shost);
          memcpy(eth->ether_dhost, arp_eth->arp_sha, sizeof(arp_eth->arp_sha));
          send_packet(x->interface, x);
          
        }
        size_q = 0;
        continue;
      }
    }

    /* icmp echo request
        check if is for router
    */

    if (ip_hdr->protocol == IPPROTO_ICMP &&
        (ip_hdr->daddr ==
         inet_addr(get_interface_ip(m.interface)))) {  // icmp protocol
      struct icmphdr *recv_icmphdr =
          (struct icmphdr *)(m.payload + sizeof(struct ether_header) +
                             sizeof(struct iphdr));
      if (recv_icmphdr->type == ICMP_ECHO) {  // ICMP_ECHO
        // turn packet into ICMP_ECHOREPLY packet
        uint32_t aux = inet_addr(get_interface_ip(m.interface));
        ip_hdr->daddr = ip_hdr->saddr;
        ip_hdr->saddr = aux;
        recv_icmphdr->type = ICMP_ECHOREPLY;
        send_packet(m.interface, &m);
        continue;
      }
    }

    if (ip_hdr->ttl <= 1) {
      // icmp for time exceeded
      struct icmphdr *ttl_icmphdr =
          (struct icmphdr *)(m.payload + sizeof(struct ether_header) +
                             sizeof(struct iphdr));

     
      char aux2[6];
      memcpy(aux2, eth_hdr->ether_shost, sizeof(eth_hdr->ether_shost));
      memcpy(eth_hdr->ether_shost, eth_hdr->ether_dhost,
             sizeof(eth_hdr->ether_shost));
      memcpy(eth_hdr->ether_dhost, aux2, sizeof(eth_hdr->ether_shost));
      eth_hdr->ether_type = htons(0x0800);
      ip_hdr->version = 4;
      ip_hdr->ihl = 5;
      ip_hdr->tos = 0;
      ip_hdr->tot_len = htons(sizeof(struct iphdr) + sizeof(struct icmphdr));
      ip_hdr->frag_off = 0;
      ip_hdr->ttl = 200;
      ip_hdr->protocol = IPPROTO_ICMP;
      uint32_t aux3 = ip_hdr->daddr;
      ip_hdr->daddr = ip_hdr->saddr;
      ip_hdr->saddr = aux3;
      ip_hdr->id = htons(getpid() & 0xFFFF);
      m.len = sizeof(struct ether_header) + sizeof(struct iphdr) +
              sizeof(struct icmphdr);

      ip_hdr->check = 0;
      ip_hdr->check = ip_checksum(ip_hdr, sizeof(struct iphdr));

      ttl_icmphdr->type = ICMP_TIME_EXCEEDED;
      ttl_icmphdr->code = 0;
      ttl_icmphdr->un.echo.id = htons(getpid() & 0xFFFF);
      ttl_icmphdr->checksum = 0;
      ttl_icmphdr->checksum = ip_checksum(ttl_icmphdr, sizeof(struct icmphdr));

      send_packet(m.interface, &m);
      continue;
    }

    uint16_t sum = ip_hdr->check;
    ip_hdr->check = 0;

    if (sum != ip_checksum(ip_hdr, sizeof(struct iphdr))) {
      fprintf(stderr, "%s\n", "Wrong checksum!");
      continue;
    }

    struct rtable_entry *next_ip =
        get_best_route(ip_hdr->daddr, 0, rtable_size);
    if (next_ip == NULL) {
      // icmp for host unreachable
      struct icmphdr *host_icmphdr =
          (struct icmphdr *)(m.payload + sizeof(struct ether_header) +
                             sizeof(struct iphdr));

      eth_hdr->ether_type = htons(0x0800);
      char aux2[6];
      memcpy(aux2, eth_hdr->ether_shost, sizeof(eth_hdr->ether_shost));
      memcpy(eth_hdr->ether_shost, eth_hdr->ether_dhost,
             sizeof(eth_hdr->ether_shost));
      memcpy(eth_hdr->ether_dhost, aux2, sizeof(eth_hdr->ether_shost));

      ip_hdr->version = 4;
      ip_hdr->ihl = 5;
      ip_hdr->tos = 0;
      ip_hdr->tot_len = htons(sizeof(struct iphdr) + sizeof(struct icmphdr));
      ip_hdr->frag_off = 0;
      ip_hdr->ttl = 200;
      ip_hdr->protocol = IPPROTO_ICMP;
      uint32_t aux3 = ip_hdr->daddr;
      ip_hdr->daddr = ip_hdr->saddr;
      ip_hdr->saddr = aux3;
      ip_hdr->id = htons(getpid() & 0xFFFF);
      m.len = sizeof(struct ether_header) + sizeof(struct iphdr) +
              sizeof(struct icmphdr);

      ip_hdr->check = 0;
      ip_hdr->check = ip_checksum(ip_hdr, sizeof(struct iphdr));

      host_icmphdr->type = ICMP_DEST_UNREACH;
      host_icmphdr->code = 0;
      host_icmphdr->un.echo.id = htons(getpid() & 0xFFFF);
      host_icmphdr->checksum = 0;
      host_icmphdr->checksum =
          ip_checksum(host_icmphdr, sizeof(struct icmphdr));

      send_packet(m.interface, &m);
      continue;
    }

    ip_hdr->ttl--;
    ip_hdr->check = 0;
    ip_hdr->check = ip_checksum(ip_hdr, sizeof(struct iphdr));

    struct arp_entry *dest = get_arp_entry(next_ip->next_hop);
    if (dest == NULL) {
      size_q++;
      packet req;
      struct ether_header *req_hdr = (struct ether_header *)req.payload;
      struct ether_arp *arp_eth =
          (struct ether_arp *)(req.payload + sizeof(struct ether_header));

      
      req.interface = next_ip->interface;
      get_interface_mac(req.interface, req_hdr->ether_shost);
      hwaddr_aton("ff:ff:ff:ff:ff:ff", req_hdr->ether_dhost);

      arp_eth->arp_op = htons(1);
      arp_eth->arp_hrd = htons(ARPHRD_ETHER);
      arp_eth->arp_pro = htons(ETHERTYPE_IP);
      arp_eth->arp_hln = 6;
      arp_eth->arp_pln = 4;
      req_hdr->ether_type = htons(0x0806);

      get_interface_mac(next_ip->interface,
                        arp_eth->arp_sha);  // arp frame sender
      memcpy(arp_eth->arp_spa, get_interface_ip(next_ip->interface),
             sizeof(get_interface_ip(
                 next_ip->interface)));  // arp sender protocol (ip)
      hwaddr_aton("00:00:00:00:00:00", arp_eth->arp_tha);
      // arp frame target address
      memcpy(arp_eth->arp_tpa, &next_ip->next_hop,
             sizeof(next_ip->next_hop));  // arp target protocol(ip)
      req.len = sizeof(struct ether_header) + sizeof(struct ether_arp);
      send_packet(req.interface, &req);

      get_interface_mac(next_ip->interface, eth_hdr->ether_shost);
      m.interface = next_ip->interface;
      packet other = m;
      queue_enq(to_send, &other);

      continue;
    }

    get_interface_mac(next_ip->interface, eth_hdr->ether_shost);
    memcpy(eth_hdr->ether_dhost, dest->mac, sizeof(dest->mac));
    send_packet(next_ip->interface, &m); 
    continue; 
  }
}
