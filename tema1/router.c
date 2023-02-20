#include "queue.h"
#include "skel.h"

void send_arp(struct ether_header *eth_header, struct arp_header *arp_header, int interface, int len)
{
	packet msg;

	msg.interface = interface;
	msg.len = len;
	memcpy(&(msg.payload), eth_header, sizeof(struct ether_header));
	struct arp_header *arp_head = (struct arp_header *)(msg.payload + sizeof(struct ether_header));
	memcpy(arp_head, arp_header, sizeof(struct arp_header));
	send_packet(&msg);
}

int parcurgere(struct iphdr *ip_hdr, struct route_table_entry *destinatie, struct route_table_entry *rtable_ent, int len_rtable)
{
	uint32_t mask_max = 0;
	int gasit = -1;
	for (int i = 0; i < len_rtable; i++)
	{
		if ((ip_hdr->daddr & rtable_ent[i].mask) == rtable_ent[i].prefix)
		{
			if (rtable_ent[i].mask >= mask_max)
			{
				gasit = i;
				*destinatie = rtable_ent[i];
				mask_max = rtable_ent[i].mask;
			}
		}
	}
	return gasit;
}

int cautare_binara(struct iphdr *ip_hdr, struct route_table_entry *destinatie, struct route_table_entry *rtable_ent, int len_rtable)
{
	int s = 0, d = len_rtable - 1;
	int m;
	uint32_t max_mask = 0;
	int idx = -1;
	while (s <= d)
	{
		if (s == d)
		{
			if ((ip_hdr->daddr & rtable_ent[s].mask) == rtable_ent[s].prefix)
			{

				if (htonl(rtable_ent[s].mask) > htonl(max_mask))
				{
					*destinatie = rtable_ent[s];
					max_mask = rtable_ent[s].mask;
					idx = s;
				}
			}
			return idx;
		}

		m = (s + d) / 2;
		if ((ip_hdr->daddr & rtable_ent[m].mask) == rtable_ent[m].prefix)
		{
			while ((ip_hdr->daddr & rtable_ent[m].mask) == rtable_ent[m].prefix && m < len_rtable)
			{
				if (htonl(rtable_ent[m].mask) > htonl(max_mask))
				{
					*destinatie = rtable_ent[m];
					max_mask = rtable_ent[m].mask;
					idx = m;
				}
				m++;
			}
			s = m;
		}
		else if (ntohl(ip_hdr->daddr) < ntohl(rtable_ent[m].prefix))
		{
			d = m - 1;
		}
		else
		{
			s = m + 1;
		}
	}

	return idx;
}

int cautare_binara_next_hop(struct arp_header *new_arp_header, struct route_table_entry *rtable_ent, int len_rtable)
{
	uint32_t mask_max = 0;
	int s = 0, d = len_rtable - 1, m;
	int idx = -1;
	while (s <= d)
	{
		if (s == d)
		{
			if (rtable_ent[s].next_hop == new_arp_header->tpa)
			{
				if (rtable_ent[s].mask > mask_max)
				{
					idx = s;
					mask_max = rtable_ent[s].mask;
				}
			}
			return idx;
		}
		m = (s + d) / 2;
		if (rtable_ent[m].next_hop == new_arp_header->tpa)
		{
			while (new_arp_header->tpa == rtable_ent[m].next_hop)
			{
				if (rtable_ent[m].mask > mask_max)
				{
					idx = m;
					mask_max = rtable_ent[m].mask;
				}
				m++;
			}
			s = m;
		}
		else if (ntohl(new_arp_header->tpa) < ntohl(rtable_ent[m].prefix))
		{
			d = m - 1;
		}
		else
		{
			s = m + 1;
		}
	}
	return idx;
}

typedef struct arpTable
{
	uint32_t ip_address;
	uint8_t mac_address[ETH_ALEN];
} TarpTable;

int comparatorf(const void *T1, const void *T2)
{
	struct route_table_entry *T1c = (struct route_table_entry *)(T1);
	struct route_table_entry *T2c = (struct route_table_entry *)(T2);
	if (ntohl(T1c->prefix) < ntohl(T2c->prefix))
	{
		return -1;
	}
	else if ((ntohl(T1c->prefix) == ntohl(T2c->prefix)))
	{
		if (ntohl(T1c->mask) < ntohl(T2c->mask))
		{
			return -1;
		}
		else
		{
			return 1;
		}
	}

	return 1;
}

int main(int argc, char *argv[])
{
	packet m;
	int rc;

	// Do not modify this line
	init(argc - 2, argv + 2);
	queue Q = queue_create();
	struct route_table_entry *rtable_ent = calloc(100000, sizeof(struct route_table_entry));
	int len_rtable = read_rtable(argv[1], rtable_ent);
	TarpTable *ArpTable = calloc(100000, sizeof(TarpTable));
	int len_ArpTable = 0;

	qsort(rtable_ent, len_rtable, sizeof(struct route_table_entry), &comparatorf);
	while (1)
	{
		rc = get_packet(&m);
		DIE(rc < 0, "get_packet");
		/* TODO */
		struct ether_header *eth_header = (struct ether_header *)(m.payload);
		uint8_t mac_address[ETH_ALEN];
		uint8_t broadcast_addr[ETH_ALEN];
		hwaddr_aton("FF:FF:FF:FF:FF:FF", broadcast_addr);
		get_interface_mac(m.interface, mac_address);
		char *ip_address = get_interface_ip(m.interface);
		struct in_addr inp;
		inet_aton(ip_address, &inp);
		uint32_t ip_addr = inp.s_addr;
		if (m.len >= sizeof(struct ether_header))
		{
			if (memcmp(eth_header->ether_dhost, broadcast_addr, 6) == 0 || memcmp(mac_address, &(eth_header->ether_dhost), sizeof(uint8_t)) == 0)
			{
				uint16_t ether_type = eth_header->ether_type;
				if (ether_type == ntohs(ETHERTYPE_ARP))
				{
					struct arp_header *msg_arp_header = (struct arp_header *)(m.payload + sizeof(struct ether_header));
					if (msg_arp_header->op == ntohs(ARPOP_REQUEST))
					{
						struct ether_header *new_et_header = calloc(1, sizeof(struct ether_header));
						memcpy(new_et_header->ether_dhost, eth_header->ether_shost, ETH_ALEN * sizeof(uint8_t));
						memcpy(new_et_header->ether_shost, mac_address, ETH_ALEN);
						new_et_header->ether_type = htons(ETHERTYPE_ARP);
						struct arp_header *new_arp_header = calloc(1, sizeof(struct arp_header));
						memcpy(new_arp_header, msg_arp_header, sizeof(struct arp_header));
						new_arp_header->op = htons(ARPOP_REPLY);
						memcpy(new_arp_header->sha, mac_address, sizeof(uint8_t) * ETH_ALEN);
						new_arp_header->spa = ip_addr;
						new_arp_header->tpa = msg_arp_header->spa;
						memcpy(new_arp_header->tha, msg_arp_header->sha, sizeof(uint8_t) * ETH_ALEN);
						int interface = -1;
						int idx = cautare_binara_next_hop(new_arp_header, rtable_ent, len_rtable);

						if (idx != -1)
						{
							interface = rtable_ent[idx].interface;
							send_arp(new_et_header, new_arp_header, interface, m.len);
						}
					}
					else if (msg_arp_header->op == ntohs(ARPOP_REPLY))
					{
						TarpTable arp_entry;
						arp_entry.ip_address = msg_arp_header->spa;
						memcpy(arp_entry.mac_address, msg_arp_header->sha, sizeof(uint8_t) * ETH_ALEN);
						ArpTable[len_ArpTable++] = arp_entry;
						if (!queue_empty(Q))
						{
							packet *msg2 = (packet *)queue_deq(Q);

							struct ether_header *eth_header_msg2 = (struct ether_header *)(msg2->payload);
							eth_header_msg2->ether_type = htons(ETHERTYPE_IP);
							memcpy(eth_header_msg2->ether_dhost, arp_entry.mac_address, sizeof(uint8_t) * ETH_ALEN);
							memcpy(eth_header_msg2->ether_shost, mac_address, ETH_ALEN);
							send_packet(msg2);
						}
					}
				}

				if (ether_type == ntohs(ETHERTYPE_IP))
				{
					struct iphdr *ip_hdr = (struct iphdr *)(m.payload + sizeof(struct ether_header));
					if (ip_hdr->daddr == ip_addr)
					{
						struct icmphdr *icmp_hdr = (struct icmphdr *)(m.payload + (sizeof(struct ether_header) + sizeof(struct iphdr)));
						if (icmp_hdr->type == 8 && icmp_hdr->code == 0)
						{
							icmp_hdr->type = 0;
							ip_hdr->daddr = ip_hdr->saddr;
							ip_hdr->saddr = ip_addr;
							icmp_hdr->checksum = 0;
							icmp_hdr->checksum = icmp_checksum((uint16_t *)icmp_hdr, sizeof(struct icmphdr));
							memcpy(eth_header->ether_dhost, eth_header->ether_shost, ETH_ALEN);
							memcpy(eth_header->ether_shost, mac_address, ETH_ALEN);
							send_packet(&m);
						}
					}
					else
					{
						uint16_t cksum = 0;
						uint16_t cksumhdr = ip_hdr->check;
						ip_hdr->check = 0;
						cksum = (ip_checksum((uint8_t *)ip_hdr, sizeof(struct iphdr)));
						if (cksum == cksumhdr)
						{

							if (ip_hdr->ttl <= 1)
							{
								packet *msg = calloc(1, sizeof(packet));
								msg->interface = m.interface;
								msg->len = sizeof(struct ether_header) + sizeof(struct iphdr) + sizeof(struct icmphdr) + 56;
								struct ether_header *ethhdr_msg = (struct ether_header *)(msg->payload);
								struct iphdr *iphdr_msg = (struct iphdr *)(msg->payload + sizeof(struct ether_header));
								struct icmphdr *icmphdr_msg = (struct icmphdr *)(msg->payload + sizeof(struct ether_header) + sizeof(struct iphdr));
								memcpy(iphdr_msg, ip_hdr, sizeof(struct iphdr));
								memcpy(ethhdr_msg->ether_dhost, eth_header->ether_shost, ETH_ALEN);
								memcpy(ethhdr_msg->ether_shost, mac_address, ETH_ALEN);
								ethhdr_msg->ether_type = eth_header->ether_type;
								iphdr_msg->daddr = ip_hdr->saddr;
								iphdr_msg->saddr = ip_addr;
								iphdr_msg->tot_len =  htons(sizeof(struct iphdr) + sizeof(struct icmphdr) + 64);
								iphdr_msg->ttl = 64;
								iphdr_msg->check = 0;
								iphdr_msg->check = ip_checksum((uint8_t *)iphdr_msg, sizeof(struct iphdr));
								icmphdr_msg->type = 11;
								icmphdr_msg->code = 0;
								memcpy(msg->payload + sizeof(struct ether_header) + sizeof(struct iphdr) + sizeof(struct icmphdr), ip_hdr, 64);
								icmphdr_msg->checksum = 0;
								icmphdr_msg->checksum = icmp_checksum((uint16_t *)icmphdr_msg, ntohs(iphdr_msg->tot_len) - sizeof(struct iphdr));
								send_packet(msg);
							}
							else
							{
								ip_hdr->ttl--;
								struct route_table_entry destinatie;

								int gasit = cautare_binara(ip_hdr, &destinatie, rtable_ent, len_rtable);

								if (gasit != -1)
								{
									ip_hdr->check = 0;
									uint16_t cksm = ip_checksum((uint8_t *)ip_hdr, sizeof(struct iphdr));
									ip_hdr->check = cksm;
									m.interface = destinatie.interface;
									TarpTable *arp_table_entry = NULL;
									for (int i = 0; i < len_ArpTable; i++)
									{
										if (destinatie.next_hop == ArpTable[i].ip_address)
										{
											arp_table_entry = &ArpTable[i];
											break;
										}
									}
									if (arp_table_entry != NULL)
									{
										memcpy(eth_header->ether_dhost, arp_table_entry->mac_address, sizeof(uint8_t) * ETH_ALEN);
										memcpy(eth_header->ether_shost, mac_address, sizeof(uint8_t) * ETH_ALEN);
										send_packet(&m);
									}
									else
									{
										eth_header = (struct ether_header *)(m.payload);
										packet *packet_cpy = calloc(1, sizeof(packet));
										memcpy(packet_cpy, &m, sizeof(packet));
										queue_enq(Q, packet_cpy);
										
										struct arp_header *new_arp_req = calloc(1, sizeof(struct arp_header));
										struct ether_header *new_eth_hdr = calloc(1, sizeof(struct ether_header));
										uint8_t mac_addr[ETH_ALEN];
										get_interface_mac(m.interface, mac_addr);
										memcpy(new_eth_hdr->ether_shost, mac_addr, ETH_ALEN * sizeof(uint8_t));
										memcpy(new_eth_hdr->ether_dhost, broadcast_addr, ETH_ALEN * sizeof(uint8_t));
										new_eth_hdr->ether_type = htons(ETHERTYPE_ARP);
										new_arp_req->htype = htons((uint16_t)(1));
										new_arp_req->ptype = htons((uint16_t)(2048));
										new_arp_req->hlen = 6;
										new_arp_req->plen = 4;
										new_arp_req->op = htons((uint16_t)ARPOP_REQUEST);
										memcpy(new_arp_req->sha, mac_addr, ETH_ALEN * sizeof(uint8_t));
										memcpy(new_arp_req->tha, broadcast_addr, ETH_ALEN);
										char *ip_address2 = get_interface_ip(m.interface);
										struct in_addr inp2;
										inet_aton(ip_address2, &inp2);
										new_arp_req->spa = (inp2.s_addr);
										new_arp_req->tpa = (destinatie.next_hop);
										send_arp(new_eth_hdr, new_arp_req, m.interface, sizeof(struct ether_header) + sizeof(struct arp_header));
									}
								}
								else
								{
									packet *msg = calloc(1, sizeof(packet));
									msg->interface = m.interface;
									msg->len = sizeof(struct ether_header) + sizeof(struct iphdr) + sizeof(struct icmphdr) + 64;
									struct ether_header *ethhdr_msg = (struct ether_header *)(msg->payload);
									struct iphdr *iphdr_msg = (struct iphdr *)(msg->payload + sizeof(struct ether_header));
									struct icmphdr *icmphdr_msg = (struct icmphdr *)(msg->payload + sizeof(struct ether_header) + sizeof(struct iphdr));
									memcpy(iphdr_msg, ip_hdr, sizeof(struct iphdr));
									memcpy(ethhdr_msg->ether_dhost, eth_header->ether_shost, ETH_ALEN);
									memcpy(ethhdr_msg->ether_shost, mac_address, ETH_ALEN);
									ethhdr_msg->ether_type = eth_header->ether_type;
									iphdr_msg->daddr = ip_hdr->saddr;
									iphdr_msg->saddr = ip_addr;
									iphdr_msg->tot_len = ntohs(sizeof(struct iphdr) + sizeof(struct icmphdr) + 64);
									iphdr_msg->ttl = 64;
									iphdr_msg->check = 0;
									iphdr_msg->check = ip_checksum((uint8_t *)iphdr_msg, sizeof(struct iphdr));
									icmphdr_msg->type = 3;
									icmphdr_msg->code = 0;
									memcpy(msg->payload + sizeof(struct ether_header) + sizeof(struct iphdr) + sizeof(struct icmphdr), ip_hdr, 64);
									icmphdr_msg->checksum = 0;
									icmphdr_msg->checksum = icmp_checksum((uint16_t *)icmphdr_msg, ntohs(iphdr_msg->tot_len) - sizeof(struct iphdr));
									send_packet(msg);
								}
							}
						}
					}
				}
			}
		}
	}
}
