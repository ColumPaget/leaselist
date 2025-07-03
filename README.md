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
  -M <path>      - path to oui.txt file containing mac-address to vendor mapping. Supplying this activates host vendor output.
  -N <path>      - path to custom names file containing mac-address to name mapping. Defaults to '/etc/macnames.conf'.
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


MAC names file
--------------

As of version 2.0 you can supply a 'names file' to set a name to be displayed against a specific MAC address. The default path of the file is `/etc/macnames.conf' and the file format is simply a mac-address followed by whitespace, followed by a custom name which can include spaces, and then the whole record ends with newline.
