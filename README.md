leaselist - display DHCP leases in logfiles or leasefiles from ISC DHCP 
-----------------------------------------------------------------------

leaselist is an application that collects and displays information about leases given out by ISC DHCP, getting information either from syslog logfiles or dhcp lease files. It can limit displayed leases to those currently active, all seen today, or all seen this week. If a path is provided to a copy of the IEEE OUI file (downloadable at: https://standards-oui.ieee.org/oui/oui.txt) it can print out the device vendor who owns each mac address found in the device list.


License
-------

leaselist is released under the Gnu Public License version 3


Install
-------

The usual:

```
./configure
make
```

will produce an executable 'leaselist' that can be installed where appropriate.


Usage
-----

```
leaselist [options] <path> <path> ...
```

'<path>' can be the paths to either syslog files that contain ISC DHCP 'DHCPACK' entries, or dhcpd.leases files.

If no path is given, leaselist tries to display leases from either `/var/lib/dhcpd/dhcpd.leases` or `/var/state/dhcp/dhcpd.leases`, if either of those exist.


Options
-------

```
  -M <path>      - path to oui.txt file containing mac-address to vendor mapping. Supplying this activates host vendor output
  -h             - print lists of historical Names/IP-Addresses of hosts
  -n             - show still-active leases
  -now           - show still-active leases
  -t             - show leases given today
  -today         - show leases given today
  -w             - show leases given this week
  -week          - show leases given this week
  -help          - print help
  --help         - print help
  -?             - print help

```


