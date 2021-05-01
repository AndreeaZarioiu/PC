#include "skel.h"

struct rtable_entry {
    uint32_t  prefix;
    uint32_t  next_hop;
    uint32_t  mask;
    int interface;
} __attribute__((packed));

struct arp_entry {
    uint32_t  ip;
    uint8_t mac[6];
} __attribute__((packed));

uint16_t ip_checksum(void *vdata, size_t length) {
  char *data = (char *)vdata;

  uint64_t acc = 0xffff;
  unsigned int offset = ((uintptr_t)data) & 3;
  if (offset) {
    size_t count = 4 - offset;
    if (count > length) count = length;
    uint32_t word = 0;
    memcpy(offset + (char *)&word, data, count);
    acc += ntohl(word);
    data += count;
    length -= count;
  }
  char *data_end = data + (length & ~3);
  while (data != data_end) {
    uint32_t word;
    memcpy(&word, data, 4);
    acc += ntohl(word);
    data += 4;
  }
  length &= 3;
  if (length) {
    uint32_t word = 0;
    memcpy(&word, data, length);
    acc += ntohl(word);
  }
  acc = (acc & 0xffffffff) + (acc >> 32);
  while (acc >> 16) {
    acc = (acc & 0xffff) + (acc >> 16);
  }
  if (offset & 1) {
    acc = ((acc & 0xff00) >> 8) | ((acc & 0x00ff) << 8);
  }
  return htons(~acc);
}

int parse_rtable(struct rtable_entry **rtable, int total) 
{
    FILE *f;
    char buffer[320];
    int bytes;
    char prefix[100];
    char next_hop[100];
    char mask[100];
    char interface[10];
    
    f = fopen("rtable.txt", "r");
    if(f == NULL) {
        fprintf(stderr, "Eroare la deschidere" "\n");
        return 0;
    }
    bytes = fgets(buffer, sizeof(buffer), f);
    int count = 0;
    while(bytes){

        // extending the table
        if(count == total){
            total *= 2;
            *rtable = realloc(*rtable, sizeof(struct rtable_entry) * total);
            
        }

        sscanf(buffer, "%s %s %s %s", prefix, next_hop, mask, interface);
        
        (*rtable)[count].prefix = inet_addr(prefix);
        (*rtable)[count].next_hop = inet_addr(next_hop);
        (*rtable)[count].mask = inet_addr(mask);
        (*rtable)[count].interface = atoi(interface);
        bytes = fgets(buffer, sizeof(buffer), f);
        
        count++;
    }
    
    fclose(f);
    
    return count;
}