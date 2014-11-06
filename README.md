changeMACAddress
================

This is a script for changing a network adapter's MAC address. Support is for Windows Vista or later. As of now it has only been tested on Windows 7. 

### How to Use

By default, `changeMACAddress.exe` modifies the MAC address of the network adapter that currently has a connection, so make sure you are connected to the internet before it is used. Alternatively, you can specify the name of the adapter you want to change with

`changeMACAddress -n <name>`

and the `.exe` will change the MAC address for that adapter.

The new MAC addresss of the adapter is pseud-randomly generated, unless specified. You can use the `-s <address>` to specify a specific address to set the MAC address to, like

`changeMACAddress -s <address>`

You can combine both the `-n` and `-s` options also:

`changeMACAddress -n <name> -s <address>`

In addition, you can also reset (disable then enable) a specified network adapter, using `-r`:

`changeMACAddress -r <name>`
