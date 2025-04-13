#ifndef LEASELIST_LOGFILE_INPUT_H
#define LEASELIST_LOGFILE_INPUT_H

#include "common.h"



/*
2025-04-10 10:03:13.0 dhcpd@ daemon.info DHCPACK on 10.2.255.76 to 44:33:4c:cc:33:16 (wlan0) via eth2
2025-04-10 10:03:15.0 dhcpd@ daemon.info DHCPINFORM from 192.168.5.252 via eth3: not authoritative for subnet 192.168.5.0
2025-04-10 10:03:31.0 dhcpd@ daemon.info DHCPINFORM from 192.168.5.253 via eth3: not authoritative for subnet 192.168.5.0
2025-04-10 10:03:41.0 dhcpd@ daemon.info DHCPREQUEST for 192.168.5.249 from 80:5e:0c:46:79:ad (SIP-T42U) via eth3
2025-04-10 10:03:41.0 dhcpd@ daemon.info DHCPACK on 192.168.5.249 to 80:5e:0c:46:79:ad (SIP-T42U) via eth3
2025-04-10 10:03:54.0 dhcpd@ daemon.info DHCPINFORM from 192.168.5.251 via eth3: not authoritative for subnet 192.168.5.0
2025-04-10 10:03:55.0 dhcpd@ daemon.info DHCPREQUEST for 10.2.255.143 from 38:87:d5:35:30:a4 (JulianLaptop) via eth2
2025-04-10 10:03:55.0 dhcpd@ daemon.info DHCPACK on 10.2.255.143 to 38:87:d5:35:30:a4 (JulianLaptop) via eth2
2025-04-10 10:03:56.0 dhcpd@ daemon.info DHCPREQUEST for 10.1.121.97 from 44:a5:6e:61:ba:e8 (AxiomLANPOE) via eth1

2025-04-10 14:05:35.0 dhcpd@ daemon.info DHCPOFFER on 10.2.255.75 to 8a:1d:ba:3b:35:dd (Julian-s-A34) via eth2
*/

int IsDate(const char *Token);
int IsTime(const char *Token);
const char *LogFileProcessDHCP(const char *Input, const char *Date, const char *Time, ListNode *Hosts);
void LogFileParseDHCPHost(ListNode *Hosts, const char *Line);
int LogFileRead(const char *Path, ListNode *Hosts);

#endif

