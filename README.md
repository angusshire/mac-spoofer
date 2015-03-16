# mac-spoofer :black_joker: 

This is a script for spoofing a network adapter's MAC address on Windows Vista/7/8.

### How it Works

adsf

### How to Use

By default, `changeMACAddress` modifies the MAC address of the network adapter that currently has a connection, so make sure you are connected to the internet before it is used. Alternatively, you can specify the name of the adapter you want to change with

`changeMACAddress -n <name>`

and the executable will set the named adapter to the new MAC address.

The new MAC addresss of the adapter is pseud-randomly generated, unless specified. You can use the `-s` option to specify a specific address to set the MAC address too, like

`changeMACAddress -s <address>`

When using the `-s` option, only valid MAC addresses are accepted. Valid MAC addressses are 12 character hexadecimal strings where the second nibble is '2', '6', 'A', or 'E'. e.g., this is a valid MAC Address argument `AAAAAAAAAAAA`.

You can combine both the `-n` and `-s` options also:

`changeMACAddress -n <name> -s <address>`

In addition, you can also reset (disable then enable) a specified network adapter, using `-r`:

`changeMACAddress -r <name>`
