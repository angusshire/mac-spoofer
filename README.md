# mac-spoofer
mac-spoofer is a script for spoofing a network adapter's MAC address on Windows 7 64-bit.

### How it Works
mac-spoofer uses the WMI API and Win32 API to retrieve your network adapter's properties, like its device id and globally unique identifier (GUID). The adapter's MAC address is modified by setting the value `NetworkAddress` in a key in the registry path `HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\Class\{4D36E972-E325-11CE-BFC1-08002bE10318}`. The registry key where `NetworkAddress` is set is found by matching your adapter's GUID to the `NetCfgInstanceId` value.

Note that the second nibble of the MAC address must be '2', '6', 'A', or 'E'. This is because the second-least significant bit of the most significant byte of the address is `1` if your address is locally administered, or `0` if it is universally administered. Universally administered addresses are assigned by the device's manufacturer, so your universally administered address is your original address. Since we are locally changing the MAC address, the second-least significant bit of the first byte must be `1`. The least significant bit of the first byte must also be `0` because your adapter is using unicast transmission (i.e., only one NIC is receiving frames).

With the second-least significant bit of the first byte as `1` and the least significant bit of the first byte as `0`, only '2', '6', 'A', or 'E' are possible hex values for the second nibble.

### How to Use

By default, mac-spoofer modifies the MAC address of the network adapter that currently has a connection, so make sure you are connected to the internet before it is used. Alternatively, you can specify the name of the adapter you want to change with

`mac-spoofer -n <name>`

and the executable will set the named adapter to the new MAC address.

The new MAC addresss of the adapter is pseudo-randomly generated, unless specified. You can use the `-s` option to specify a specific address to set the MAC address too, like

`mac-spoofer -s <address>`

When using the `-s` option, only valid MAC addresses are accepted. Valid MAC addressses are 12 character hexadecimal strings where the second nibble is '2', '6', 'A', or 'E'. e.g., this is a valid MAC Address argument `AAAAAAAAAAAA`.

You can combine both the `-n` and `-s` options also:

`mac-spoofer -n <name> -s <address>`

In addition, you can also reset (disable then enable) a specified network adapter, using `-r`:

`mac-spoofer -r <name>`

### Platforms
This script supports only Windows 7 64-bit. You can try on other versions of Windows to see if it works. If all else fails, you can change it manually. See this [link](http://www.howtogeek.com/192173/how-and-why-to-change-your-mac-address-on-windows-linux-and-mac/) for how to do so.

If you're on a Mac changing your MAC address can be done with just one line in the terminal: 

`sudo ifconfig en0 ether $(perl -e 'for ($i=0;$i<5;$i++){@m[$i]=int(rand(256));} printf "02:%X:%X:%X:%X:%X\n",@m;') && sudo ifconfig en0 down && sudo ifconfig en0 up`

### License
mac-spoof is distributed under the GNU General Public License v3.0 (GPLv3).
